// LAF OS Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/win/window.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <windowsx.h>

#include <algorithm>
#include <sstream>

#include "base/base.h"
#include "base/debug.h"
#include "base/file_content.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/string.h"
#include "base/thread.h"
#include "base/win/comptr.h"
#include "base/win/registry.h"
#include "base/win/win32_exception.h"
#include "gfx/border.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "os/event.h"
#include "os/native_cursor.h"
#include "os/win/color_space.h"
#include "os/win/keys.h"
#include "os/win/screen.h"
#include "os/win/system.h"
#include "os/win/window_dde.h"
#include "os/window_spec.h"

#include <algorithm>

#define KEY_TRACE(...)
#define MOUSE_TRACE(...)
#define TOUCH_TRACE(...)

#define kFingerAsMouseTimeout 50

// Gets the window client are in absolute/screen coordinates
#define ABS_CLIENT_RC(rc)                                                                          \
  RECT rc;                                                                                         \
  GetClientRect(m_hwnd, &rc);                                                                      \
  MapWindowPoints(m_hwnd, NULL, (POINT*)&rc, 2)

#ifndef INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN
  #define INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN 1
#endif

#ifndef INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK
  #define INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK_OFF 0
#endif

namespace os {

// Converts an os::Hit to a Win32 hit test value
static int hit2hittest[] = {
  HTNOWHERE,     // os::Hit::None
  HTCLIENT,      // os::Hit::Content
  HTCAPTION,     // os::Hit::TitleBar
  HTTOPLEFT,     // os::Hit::TopLeft
  HTTOP,         // os::Hit::Top
  HTTOPRIGHT,    // os::Hit::TopRight
  HTLEFT,        // os::Hit::Left
  HTRIGHT,       // os::Hit::Right
  HTBOTTOMLEFT,  // os::Hit::BottomLeft
  HTBOTTOM,      // os::Hit::Bottom
  HTBOTTOMRIGHT, // os::Hit::BottomRight
  HTMINBUTTON,   // os::Hit::MinimizeButton
  HTMAXBUTTON,   // os::Hit::MaximizeButton
  HTCLOSE,       // os::Hit::CloseButton
};

static int hit2hittest_entries = sizeof(hit2hittest) / sizeof(hit2hittest[0]);

static PointerType wt_packet_pkcursor_to_pointer_type(int pkCursor)
{
  switch (pkCursor % 3) {
    case 0: return PointerType::Cursor;
    case 1: return PointerType::Pen;
    case 2: return PointerType::Eraser;
    default:
      // TODO check if pkCursor=6 to notify about an inverted
      //      stylus/eraser if we enable EnableMouseInPointer()
      break;
  }
  // Impossible case (negative pkCursor?)
  ASSERT(false);
  // Return just "pen" for packets from unknown devices (just to keep
  // the pressure information).
  return PointerType::Pen;
}

static inline bool same_mouse_event(Event& a, Event& b)
{
  return (a.type() == b.type() && a.position() == b.position() && a.modifiers() == b.modifiers() &&
          a.button() == b.button() && a.pointerType() == b.pointerType() &&
          a.pressure() == b.pressure());
}

static BOOL CALLBACK log_monitor_info(HMONITOR monitor, HDC hdc, LPRECT rc, LPARAM lparam)
{
  MONITORINFOEXA mi;
  memset((void*)&mi, 0, sizeof(mi));
  mi.cbSize = sizeof(mi);
  if (GetMonitorInfoA(monitor, &mi)) {
    std::string iccFilename = get_hmonitor_icc_filename(monitor);

    auto rc = mi.rcMonitor;
    LOG("WIN: - Monitor %dx%d%s: %s (icc=%s)\n",
        rc.right - rc.left,
        rc.bottom - rc.top,
        (mi.dwFlags & MONITORINFOF_PRIMARY ? " (primary)" : ""),
        mi.szDevice,
        iccFilename.c_str());
  }
  return TRUE;
}

std::wstring get_wnd_class_name()
{
  if (auto sys = instance()) {
    if (!sys->appName().empty())
      return base::from_utf8(sys->appName());
  }
  // On Windows the class name cannot be empty, if we use an empty
  // class the window class registration will fail.
  return std::wstring(L"laf");
}

// Keys used to detect if the Windows 11 dark mode is selected.
static constexpr const char* kPersonalizeKey =
  "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
static constexpr const char* kAppsUseLightThemeValue = "AppsUseLightTheme";

PointerType WindowWin::m_pointerType = PointerType::Unknown;
float WindowWin::m_pressure = 0.0f;
std::vector<PACKET> WindowWin::m_packets;
Event WindowWin::m_lastWintabEvent;

WindowWin::Touch::Touch() : fingers(0), canBeMouse(false), asMouse(false), timerID(0)
{
}

WindowWin::WindowWin(const WindowSpec& spec)
  : m_clientSize(1, 1)
  , m_scale(spec.scale())
  , m_baseScale(spec.scale())
  , m_isCreated(false)
  , m_adjustShadow(true)
  , m_translateDeadKeys(false)
  , m_hasMouse(false)
  , m_captureMouse(false)
  , m_usePointerApi(false)
  , m_ictx(nullptr)
  , m_ignoreRandomMouseEvents(0)
  // True by default, we prefer to interpret one finger as mouse movement
  , m_touch(new Touch)
#if OS_USE_POINTER_API_FOR_MOUSE
  , m_emulateDoubleClick(false)
  , m_doubleClickMsecs(GetDoubleClickTime())
  , m_lastPointerDownTime(0)
  , m_lastPointerDownButton(Event::NoneButton)
  , m_pointerDownCount(0)
#endif
  , m_hpenctx(nullptr)
  , m_fullscreen(false)
  , m_titled(spec.titled())
  , m_borderless(spec.borderless())
  , m_fixingPos(false)
{
  auto& winApi = system()->winApi();
  if (
#if OS_USE_POINTER_API_FOR_MOUSE
    winApi.EnableMouseInPointer && winApi.IsMouseInPointerEnabled &&
#endif
    winApi.GetPointerInfo && winApi.GetPointerPenInfo) {
    // Do not enable pointer API for mouse events because:
    // - Wacom driver doesn't inform their messages in a correct
    //   pointer API format (events from pen are reported as mouse
    //   events and without eraser tip information).
    // - We have to emulate the double-click for the regular mouse
    //   (search for m_emulateDoubleClick).
    // - Double click with Wacom stylus doesn't work.
#if OS_USE_POINTER_API_FOR_MOUSE
    if (!winApi.IsMouseInPointerEnabled()) {
      // Prefer pointer messages (WM_POINTER*) since Windows 8 instead
      // of mouse messages (WM_MOUSE*)
      winApi.EnableMouseInPointer(TRUE);
      m_emulateDoubleClick = (winApi.IsMouseInPointerEnabled() ? true : false);
    }
#endif

    // Initialize a Interaction Context to convert WM_POINTER messages
    // into gestures processed by handleInteractionContextOutput().
    if (winApi.CreateInteractionContext && winApi.RegisterOutputCallbackInteractionContext &&
        winApi.SetInteractionConfigurationInteractionContext) {
      HRESULT hr = winApi.CreateInteractionContext(&m_ictx);
      if (SUCCEEDED(hr)) {
        hr = winApi.RegisterOutputCallbackInteractionContext(
          m_ictx,
          &WindowWin::staticInteractionContextCallback,
          this);
      }
      if (SUCCEEDED(hr)) {
        INTERACTION_CONTEXT_CONFIGURATION cfg[] = {
          { INTERACTION_ID_MANIPULATION,
           INTERACTION_CONFIGURATION_FLAG_MANIPULATION |
              INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X |
              INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y |
              INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING |
              INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_INERTIA |
              INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING_INERTIA                 },
          { INTERACTION_ID_TAP,
           INTERACTION_CONFIGURATION_FLAG_TAP | INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE },
          { INTERACTION_ID_SECONDARY_TAP, INTERACTION_CONFIGURATION_FLAG_SECONDARY_TAP    },
          { INTERACTION_ID_HOLD,          INTERACTION_CONFIGURATION_FLAG_NONE             },
          { INTERACTION_ID_DRAG,          INTERACTION_CONFIGURATION_FLAG_NONE             },
          { INTERACTION_ID_CROSS_SLIDE,   INTERACTION_CONFIGURATION_FLAG_NONE             }
        };
        hr = winApi.SetInteractionConfigurationInteractionContext(
          m_ictx,
          sizeof(cfg) / sizeof(INTERACTION_CONTEXT_CONFIGURATION),
          cfg);
      }
      if (SUCCEEDED(hr)) {
        hr = winApi.SetPropertyInteractionContext(
          m_ictx,
          INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS,
          INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN);
      }
      // Disable the Windows Ink circle feedback
      if (SUCCEEDED(hr)) {
        hr = winApi.SetPropertyInteractionContext(
          m_ictx,
          INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK,
          INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK_OFF);
      }
    }

    m_usePointerApi = true;
  }

  registerClass();

  // The HWND returned by CreateWindowEx() is different than the
  // HWND used in WM_CREATE message.
  m_hwnd = createHwnd(this, spec);
  if (!m_hwnd)
    throw std::runtime_error("Error creating window");

  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  if (winApi.GetWindowDpiAwarenessContext) {
    double scaleFactor = winApi.GetDpiForWindow(m_hwnd) / (double) USER_DEFAULT_SCREEN_DPI;
    m_scale = std::max(1, (int)(m_baseScale * scaleFactor + 0.51));
  }

  // This flag is used to avoid calling T::resizeImpl() when we
  // add the scrollbars to the window. (As the T type could not be
  // fully initialized yet.)
  m_isCreated = true;

  // We activate main windows by default only (TODO we could add a
  // WindowSpec::activate() flag for this)
  m_activate = (spec.parent() == nullptr);

  // Log information about the system (for debugging/user support
  // purposes in case the window doesn't display anything)
  if (base::get_log_level() >= INFO) {
    LOG("WIN: Clean boot: %d\n", GetSystemMetrics(SM_CLEANBOOT));
    LOG("WIN: Special modes:%s%s%s%s%s%s\n",
        GetSystemMetrics(SM_MOUSEPRESENT) ? " Mouse" : "",
        GetSystemMetrics(SM_SWAPBUTTON) ? " SwappedButtons" : "",
        GetSystemMetrics(SM_TABLETPC) ? " TabletPC" : "",
        GetSystemMetrics(SM_DIGITIZER) ? " Digitizer" : "",
        GetSystemMetrics(SM_SYSTEMDOCKED) ? " Docked" : "",
        GetSystemMetrics(SM_IMMENABLED) ? " IMM" : "");
    LOG("WIN: Monitors: %d%s:\n",
        GetSystemMetrics(SM_CMONITORS),
        GetSystemMetrics(SM_SAMEDISPLAYFORMAT) ? ", same display format" : "");
    EnumDisplayMonitors(nullptr, nullptr, log_monitor_info, 0);
  }

  // TODO check if this is correct, or if Windows still need the
  //      scroll bars even when we use the pointers API, anyway at the
  //      moment we are always using the hack
  if (useScrollBarsHack()) {
    // Set scroll info to receive WM_HSCROLL/VSCROLL events (events
    // generated by some trackpad drivers).
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    si.nMin = 0;
    si.nPos = 50;
    si.nMax = 100;
    si.nPage = 10;
    SetScrollInfo(m_hwnd, SB_HORZ, &si, FALSE);
    SetScrollInfo(m_hwnd, SB_VERT, &si, FALSE);
  }
}

WindowWin::~WindowWin()
{
  auto sys = system();

  // If this assert fails it's highly probable that an os::WindowRef
  // was kept alive in some kind of memory leak (or just inside an
  // os::Event in the os::EventQueue). Also this can happen when
  // declaring a os::WindowRef before calling os::make_system(),
  // because of deletion order when destructors got called.
  ASSERT(sys);

  if (sys) {
    auto& winApi = sys->winApi();
    if (m_ictx && winApi.DestroyInteractionContext)
      winApi.DestroyInteractionContext(m_ictx);
  }

  if (m_hwnd)
    DestroyWindow(m_hwnd);
}

os::ScreenRef WindowWin::screen() const
{
  if (m_hwnd) {
    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    return os::make_ref<ScreenWin>(monitor);
  }
  else
    return os::instance()->mainScreen();
}

os::ColorSpaceRef WindowWin::colorSpace() const
{
  if (os::instance())
    if (auto defaultCS = os::instance()->windowsColorSpace())
      return defaultCS;

  if (m_hwnd) {
    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    std::string iccFilename = get_hmonitor_icc_filename(monitor);
    if (m_lastICCProfile != iccFilename) {
      m_lastICCProfile = iccFilename;
      if (!iccFilename.empty())
        m_lastColorProfile = get_colorspace_from_icc_file(iccFilename);
    }
  }
  // sRGB by default
  if (!m_lastColorProfile)
    m_lastColorProfile = os::instance()->makeColorSpace(gfx::ColorSpace::MakeSRGB());
  return m_lastColorProfile;
}

void WindowWin::setScale(int scale)
{
  double scaleFactor = m_scale / (double) m_baseScale;
  m_baseScale = scale;
  // round to nearest integer, rounding 0.5 up
  m_scale = std::max(1, (int)(scale * scaleFactor + 0.51));

  // Align window size to new scale
  {
    RECT rc;
    GetWindowRect(m_hwnd, &rc);
    SendMessage(m_hwnd, WM_SIZING, 0, (LPARAM)&rc);
    SetWindowPos(m_hwnd,
                 nullptr,
                 rc.left,
                 rc.top,
                 rc.right - rc.left,
                 rc.bottom - rc.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  }

  onResize(m_clientSize);
}

bool WindowWin::isVisible() const
{
  return (IsWindowVisible(m_hwnd) ? true : false);
}

void WindowWin::setVisible(bool visible)
{
  if (visible) {
    if (m_activate)
      ShowWindow(m_hwnd, SW_SHOWNORMAL);
    else
      ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);

    UpdateWindow(m_hwnd);
  }
  else
    ShowWindow(m_hwnd, SW_HIDE);
}

void WindowWin::activate()
{
  SetActiveWindow(m_hwnd);
}

void WindowWin::maximize()
{
  if (!isMaximized())
    ShowWindow(m_hwnd, SW_MAXIMIZE);
  else
    ShowWindow(m_hwnd, SW_RESTORE);
}

void WindowWin::minimize()
{
  ShowWindow(m_hwnd, SW_MINIMIZE);
}

bool WindowWin::isMaximized() const
{
  return (IsZoomed(m_hwnd) ? true : false);
}

bool WindowWin::isMinimized() const
{
  return (GetWindowLong(m_hwnd, GWL_STYLE) & WS_MINIMIZE ? true : false);
}

bool WindowWin::isTransparent() const
{
  return (GetWindowLong(m_hwnd, GWL_EXSTYLE) & WS_EX_LAYERED ? true : false);
}

bool WindowWin::isFullscreen() const
{
  return m_fullscreen;
}

void WindowWin::setFullscreen(bool state)
{
  const bool oldFullscreen = isFullscreen();
  m_fullscreen = state;

  // Enter into full screen mode
  if (!oldFullscreen && state) {
    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor)
      return; // No monitor?

    MONITORINFOEXA mi;
    memset((void*)&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoA(monitor, &mi))
      return; // Invalid monitor info?

    // Save the current window frame position to restore it when we
    // exit the full screen mode.
#if 0
    m_restoredPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(m_hwnd, &m_restoredPlacement);
#else
    {
      RECT rc;
      GetWindowRect(m_hwnd, &rc);
      m_restoredFrame = gfx::Rect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }
#endif

    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME);
    SetWindowLong(m_hwnd, GWL_STYLE, style);
    SetWindowPos(m_hwnd,
                 nullptr,
                 mi.rcMonitor.left,
                 mi.rcMonitor.top,
                 (mi.rcMonitor.right - mi.rcMonitor.left),
                 (mi.rcMonitor.bottom - mi.rcMonitor.top),
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  }
  // Exit from full screen mode
  else if (oldFullscreen && !state) {
    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    if (m_titled)
      style |= WS_CAPTION;
    style |= WS_THICKFRAME;
    SetWindowLong(m_hwnd, GWL_STYLE, style);

    // On restore, resize to the previous saved rect size.
#if 0
    SetWindowPlacement(m_hwnd, &m_restoredPlacement);
#else
    SetWindowPos(m_hwnd,
                 nullptr,
                 m_restoredFrame.x,
                 m_restoredFrame.y,
                 m_restoredFrame.w,
                 m_restoredFrame.h,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
  }

  notifyFullScreenStateToShell();
  onResize(m_clientSize);
}

gfx::Size WindowWin::clientSize() const
{
  return m_clientSize;
}

gfx::Rect WindowWin::frame() const
{
  RECT rc;
  BOOL withShadow = false;
  if ((DwmIsCompositionEnabled(&withShadow) != S_OK) || !withShadow ||
      // DwmGetWindowAttribute() returns the true bounds from the
      // frame edges (not from the shadow) when the DWM composition is
      // enabled.
      (DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(RECT)) != S_OK)) {
    // In other case we can just use the GetWindowRect() function.
    GetWindowRect(m_hwnd, &rc);
  }
  return gfx::Rect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
}

void WindowWin::setFrame(const gfx::Rect& _bounds)
{
  gfx::Rect bounds = _bounds;

  // If the Desktop Window Manager (DWM) composition is enabled, the
  // window has an extra shadown, so we have to adjust the given
  // "_bounds" (which represent the frame edges) with the extra shadow
  // size. This is because SetWindowPos() receives the bounds of the
  // frame from the shadow (not from the window edges).
  RECT inner, outer;
  BOOL withShadow = false;
  if ((DwmIsCompositionEnabled(&withShadow) == S_OK) && withShadow &&
      (DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &inner, sizeof(RECT)) == S_OK)) {
    GetWindowRect(m_hwnd, &outer);
    bounds.enlarge(gfx::Border(inner.left - outer.left,
                               inner.top - outer.top,
                               outer.right - inner.right,
                               outer.bottom - inner.bottom));
  }
  SetWindowPos(m_hwnd,
               nullptr,
               bounds.x,
               bounds.y,
               bounds.w,
               bounds.h,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

gfx::Rect WindowWin::contentRect() const
{
  RECT rc;
  GetClientRect(m_hwnd, &rc);
  ClientToScreen(m_hwnd, (LPPOINT)&rc);
  return gfx::Rect(rc.left, rc.top, rc.right, rc.bottom);
}

gfx::Rect WindowWin::restoredFrame() const
{
  return m_restoredFrame;
}

std::string WindowWin::title() const
{
  int n = GetWindowTextLength(m_hwnd);
  if (!n)
    return std::string();

  // One extra char for the trailing zero '\0'
  ++n;
  std::vector<wchar_t> buf(n, 0);
  GetWindowText(m_hwnd, &buf[0], n);
  return base::to_utf8(&buf[0], n);
}

void WindowWin::setTitle(const std::string& title)
{
  SetWindowText(m_hwnd, base::from_utf8(title).c_str());
}

void WindowWin::captureMouse()
{
  m_captureMouse = true;

  if (GetCapture() != m_hwnd) {
    MOUSE_TRACE("SetCapture\n");
    SetCapture(m_hwnd);
  }
}

void WindowWin::releaseMouse()
{
  m_captureMouse = false;

  if (GetCapture() == m_hwnd) {
    MOUSE_TRACE("ReleaseCapture\n");
    ReleaseCapture();
  }
}

void WindowWin::setMousePosition(const gfx::Point& position)
{
  POINT pos = { position.x * m_scale, position.y * m_scale };
  ClientToScreen(m_hwnd, &pos);
  SetCursorPos(pos.x, pos.y);

  system()->_setInternalMousePosition(gfx::Point(pos.x, pos.y));
}

void WindowWin::onSetDragTarget()
{
  HRESULT result;
  // Drag target was set, then register for drag and drop.
  if (hasDragTarget()) {
    m_dragTargetAdapter = std::make_unique<DragTargetAdapter>(this);
    result = RegisterDragDrop(m_hwnd, reinterpret_cast<LPDROPTARGET>(m_dragTargetAdapter.get()));
    if (result != S_OK) {
      LOG(LogLevel::ERROR, "Could not register window for Drag & Drop: %d\n", result);
    }
  }
  else { // Drag target was unset (set to nullptr), then revoke DnD registration.
    m_dragTargetAdapter = nullptr;
    result = RevokeDragDrop(m_hwnd);
    if (result != S_OK) {
      LOG(LogLevel::ERROR, "Could not revoke Drag & Drop: %d\n", result);
    }
  }
}

bool WindowWin::setCursor(NativeCursor cursor)
{
  HCURSOR hcursor = NULL;

  switch (cursor) {
    case NativeCursor::Hidden:
      // Do nothing, just set to null
      break;
    case NativeCursor::Arrow:     hcursor = LoadCursor(NULL, IDC_ARROW); break;
    case NativeCursor::Crosshair: hcursor = LoadCursor(NULL, IDC_CROSS); break;
    case NativeCursor::IBeam:     hcursor = LoadCursor(NULL, IDC_IBEAM); break;
    case NativeCursor::Wait:      hcursor = LoadCursor(NULL, IDC_WAIT); break;
    case NativeCursor::Link:      hcursor = LoadCursor(NULL, IDC_HAND); break;
    case NativeCursor::Help:      hcursor = LoadCursor(NULL, IDC_HELP); break;
    case NativeCursor::Forbidden: hcursor = LoadCursor(NULL, IDC_NO); break;
    case NativeCursor::Move:      hcursor = LoadCursor(NULL, IDC_SIZEALL); break;
    case NativeCursor::SizeN:
    case NativeCursor::SizeNS:
    case NativeCursor::SizeS:     hcursor = LoadCursor(NULL, IDC_SIZENS); break;
    case NativeCursor::SizeE:
    case NativeCursor::SizeW:
    case NativeCursor::SizeWE:    hcursor = LoadCursor(NULL, IDC_SIZEWE); break;
    case NativeCursor::SizeNW:
    case NativeCursor::SizeSE:    hcursor = LoadCursor(NULL, IDC_SIZENWSE); break;
    case NativeCursor::SizeNE:
    case NativeCursor::SizeSW:    hcursor = LoadCursor(NULL, IDC_SIZENESW); break;
  }

  return setCursor(hcursor, nullptr);
}

bool WindowWin::setCursor(const CursorRef& cursor)
{
  ASSERT(cursor);
  if (!cursor)
    return false;

  if (cursor->nativeHandle())
    return setCursor((HCURSOR)cursor->nativeHandle(), cursor);
  else
    return setCursor(nullptr, nullptr); // Like NativeCursor::Hidden
}

void WindowWin::performWindowAction(const WindowAction action, const Event* event)
{
  int ht = HTNOWHERE;

  switch (action) {
    case WindowAction::Move:                  ht = HTCAPTION; break;
    case WindowAction::ResizeFromTopLeft:     ht = HTTOPLEFT; break;
    case WindowAction::ResizeFromTop:         ht = HTTOP; break;
    case WindowAction::ResizeFromTopRight:    ht = HTTOPRIGHT; break;
    case WindowAction::ResizeFromLeft:        ht = HTLEFT; break;
    case WindowAction::ResizeFromRight:       ht = HTRIGHT; break;
    case WindowAction::ResizeFromBottomLeft:  ht = HTBOTTOMLEFT; break;
    case WindowAction::ResizeFromBottom:      ht = HTBOTTOM; break;
    case WindowAction::ResizeFromBottomRight: ht = HTBOTTOMRIGHT; break;
  }

  if (ht != HTNOWHERE) {
    POINT pos;
    if (event) {
      pos.x = event->position().x;
      pos.y = event->position().y;
      ClientToScreen(m_hwnd, &pos);
    }
    else {
      GetCursorPos(&pos);
    }
    // Cannot use SendMessage() because if m_borderless is true,
    // WM_NCLBUTTONDOWN will generate a MouseDown but not call the
    // original DefWindowProc().
    DefWindowProc(m_hwnd, WM_NCLBUTTONDOWN, ht, MAKELPARAM(pos.x, pos.y));
  }
}

void WindowWin::invalidateRegion(const gfx::Region& rgn)
{
#if 1 // Invalidating the region generates a flicker in Aseprite's
      // BrushPreview, because it looks like regions are then painted
      // and refreshed on the screen without synchronization (without
      // vsync?) or double-buffering (not even WS_EX_COMPOSITED fixes
      // the issue).
      //
      // Anyway we're going to give a try to this fix to improve the
      // performance in high-resolutions and fix the BrushPreview
      // later with an alternative solution.
  HRGN hrgn = nullptr;
  for (const gfx::Rect& rc : rgn) {
    HRGN rcHrgn =
      CreateRectRgn(rc.x * m_scale, rc.y * m_scale, rc.x2() * m_scale, rc.y2() * m_scale);
    if (!hrgn)
      hrgn = rcHrgn;
    else {
      CombineRgn(hrgn, hrgn, rcHrgn, RGN_OR);
      DeleteObject(rcHrgn);
    }
  }
  if (hrgn) {
    InvalidateRgn(m_hwnd, hrgn, FALSE);
    DeleteObject(hrgn);
  }
#elif 0 // Same flicker
  gfx::Rect bounds = rgn.bounds();
  RECT rc = { bounds.x * m_scale,
              bounds.y * m_scale,
              bounds.x * m_scale + bounds.w * m_scale,
              bounds.y * m_scale + bounds.h * m_scale };
  InvalidateRect(m_hwnd, &rc, FALSE);
#else   // Only way to avoid the flicker is invalidating the whole window.
        // TODO we should review this, it's almost sure that flicker
        //      is a problem from our side and there is a better way
        //      to handle it.
  InvalidateRect(m_hwnd, nullptr, FALSE);
#endif
}

std::string WindowWin::getLayout()
{
  WINDOWPLACEMENT wp;
  wp.length = sizeof(WINDOWPLACEMENT);
  if (GetWindowPlacement(m_hwnd, &wp)) {
    // clang-format off
    std::ostringstream s;
    s << 1 << ' '
      << wp.flags << ' '
      << wp.showCmd << ' '
      << wp.ptMinPosition.x << ' '
      << wp.ptMinPosition.y << ' '
      << wp.ptMaxPosition.x << ' '
      << wp.ptMaxPosition.y << ' '
      << wp.rcNormalPosition.left << ' '
      << wp.rcNormalPosition.top << ' '
      << wp.rcNormalPosition.right << ' '
      << wp.rcNormalPosition.bottom;
    return s.str();
    // clang-format on
  }
  return "";
}

void WindowWin::setLayout(const std::string& layout)
{
  WINDOWPLACEMENT wp;
  wp.length = sizeof(WINDOWPLACEMENT);

  std::istringstream s(layout);
  int ver;
  s >> ver;
  if (ver == 1) {
    // clang-format off
    s >> wp.flags
      >> wp.showCmd
      >> wp.ptMinPosition.x
      >> wp.ptMinPosition.y
      >> wp.ptMaxPosition.x
      >> wp.ptMaxPosition.y
      >> wp.rcNormalPosition.left
      >> wp.rcNormalPosition.top
      >> wp.rcNormalPosition.right
      >> wp.rcNormalPosition.bottom;
    // clang-format on
  }
  else
    return;

  if (SetWindowPlacement(m_hwnd, &wp)) {
    // TODO use the return value
  }
}

void WindowWin::setTranslateDeadKeys(bool state)
{
  m_translateDeadKeys = state;

  // Here we clear dead keys so we don't get those keys in the new
  // "translate dead keys" state. E.g. If we focus a text entry
  // field and the translation of dead keys is enabled, we don't
  // want to get previous dead keys. The same in case we leave the
  // text field with a pending dead key, that dead key must be
  // discarded.
  VkToUnicode tu;
  if (tu) {
    tu.toUnicode(VK_SPACE, 0);
    if (tu.size() != 0)
      tu.toUnicode(VK_SPACE, 0);
  }
}

void WindowWin::setInterpretOneFingerGestureAsMouseMovement(bool state)
{
  if (state) {
    if (!m_touch)
      m_touch = new Touch;
  }
  else if (m_touch) {
    killTouchTimer();
    delete m_touch;
    m_touch = nullptr;
  }
}

void WindowWin::onTabletOptionsChange()
{
  LOG("WIN: On window %p tablet options change tablet API=%d\n", m_hwnd, int(tabletAPI()));

  closeWintabCtx();
  openWintabCtx();
}

bool WindowWin::setCursor(HCURSOR hcursor, const CursorRef& cursor)
{
  SetCursor(hcursor);
  m_hcursor = hcursor;
  m_cursor = cursor;
  return (hcursor ? true : false);
}

LRESULT WindowWin::wndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
    case WM_CREATE: {
      LOG("WIN: Creating window %p (tablet API %d)\n", m_hwnd, int(tabletAPI()));
      openWintabCtx();

      if (m_borderless &&
          // Don't use drop shadow effect for borderless + transparent
          !isTransparent()) {
        BOOL dwmEnabled = false;
        if ((DwmIsCompositionEnabled(&dwmEnabled) == S_OK) && dwmEnabled) {
          // Without this, we lost the shadow effect when WM_NCCALCSIZE returns 0
          MARGINS margins = { 0, 0, 0, 1 };
          DwmExtendFrameIntoClientArea(m_hwnd, &margins);

#if 1
          // Don't render anything related to the non-client area in
          // borderless windows (with this option the Windows 11
          // rounded borders disappear)
          DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
          DwmSetWindowAttribute(m_hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));

#else // The DWMWCP_DONOTROUND option doesn't fully work on Windows 11,
      // we still need to set DWMNCRP_DISABLED for DWMWA_NCRENDERING_POLICY
      // to disable a 1-pixel border in the non-client area.

          // TODO Use the Windows 11 SDK types/constants
          uint32_t cornerPref = 1; // DWMWCP_DONOTROUND;
          DwmSetWindowAttribute(m_hwnd,
                                33, // DWMWA_WINDOW_CORNER_PREFERENCE,
                                &cornerPref,
                                sizeof(cornerPref));
#endif
        }
      }

      checkDarkModeChange();
      notifyFullScreenStateToShell();
      break;
    }

    case WM_DESTROY:
      LOG("WIN: Destroying window %p (pen context %p)\n", m_hwnd, m_hpenctx);
      closeWintabCtx();
      break;

    case WM_SHOWWINDOW:
      if (wparam)
        checkColorSpaceChange();
      break;

    case WM_WINDOWPOSCHANGING: {
      if (m_adjustShadow) {
        // Check the drop shadow size
        BOOL dwmEnabled = false;
        if ((DwmIsCompositionEnabled(&dwmEnabled) == S_OK) && dwmEnabled) {
          RECT rc, exrc;
          GetWindowRect(m_hwnd, &rc);
          if (DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &exrc, sizeof(RECT)) !=
              S_OK)
            exrc = rc;

          const int leftEdge = exrc.left - rc.left;
          const int topEdge = exrc.top - rc.top;
          const int rightEdge = rc.right - exrc.right;
          const int bottomEdge = rc.bottom - exrc.bottom;

          if (leftEdge || topEdge || rightEdge || bottomEdge) {
            WINDOWPOS* winPos = (WINDOWPOS*)lparam;

            // Add the shadow edge to the position
            winPos->x = rc.left - leftEdge;
            winPos->y = rc.top - topEdge;
            winPos->cx = rc.right - rc.left + leftEdge + rightEdge;
            winPos->cy = rc.bottom - rc.top + topEdge + bottomEdge;
            winPos->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
            m_adjustShadow = false;
            return 0;
          }
        }
        else {
          m_adjustShadow = false;
        }
      }
      break;
    }

    case WM_WINDOWPOSCHANGED:
      // When a custom frame window (borderless) is maximized, Windows
      // put the window in the monitor bounds (not the workarea), so
      // the task bar is not visible. To fix this we put the window in
      // the workarea explicitly.
      //
      // This is combined with the handling of WM_NCCALCSIZE message.
      if (m_borderless && isMaximized() && !isFullscreen() && !m_fixingPos) {
        gfx::Rect wa = screen()->workarea();

        // TODO when we maximize a window from the restored/regular
        //      state, autohide taskbars are hidden, but when come
        //      back from the fullscreen mode to the maximize mode,
        //      the autohide taskbar is visible (which is the correct
        //      behavior).

        m_fixingPos = true;
        SetWindowPos(m_hwnd,
                     nullptr,
                     wa.x,
                     wa.y,
                     wa.w,
                     wa.h,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        m_fixingPos = false;
      }
      break;

    case WM_DPICHANGED: {
      double scaleFactor = HIWORD(wparam) / (double) USER_DEFAULT_SCREEN_DPI;
      m_scale = std::max(1, (int)(m_baseScale * scaleFactor + 0.51));

      RECT* prcNewWindow = (RECT*)lparam;
      SetWindowPos(m_hwnd,
                  nullptr,
                  prcNewWindow ->left,
                  prcNewWindow ->top,
                  prcNewWindow->right - prcNewWindow->left,
                  prcNewWindow->bottom - prcNewWindow->top,
                  SWP_NOZORDER | SWP_NOACTIVATE);
      break;
    }

    case WM_SETCURSOR:
      // We set our custom cursor if we are in the client area, or in
      // the case of windows with custom frames (borderless), we
      // always set our own cursor.
      if (LOWORD(lparam) == HTCLIENT || m_borderless) {
        SetCursor(m_hcursor);
        return TRUE;
      }
      break;

    case WM_CLOSE: {
      Event ev;
      ev.setType(Event::CloseWindow);
      queueEvent(ev);

      // Don't close the window, it must be closed manually after
      // the CloseWindow event is processed.
      return 0;
    }

    case WM_NCPAINT:
      if (m_borderless) {
        // Don't paint frame border/grid grip in the scrollbars.
        return 0;
      }
      break;

    case WM_ERASEBKGND:
      // Don't erase background to avoid any kind of flickering
      return TRUE;

    case WM_NCACTIVATE:
      // If the user is activating a child window, the title bar of
      // the parent must be kept active. We control this when the
      // parent receives a WM_NCACTIVATE with wparam==FALSE to redraw
      // its title bar as inactive.
      if (!wparam && lparam != -1) {
        HWND possibleChild = reinterpret_cast<HWND>(lparam);
        if (IsWindow(possibleChild) && GetAncestor(possibleChild, GA_ROOTOWNER) == m_hwnd) {
          // Keep parent title bar active.
          wparam = TRUE;
        }
      }

      // The default WM_NCACTIVATE behavior paints the default NC
      // frame borders (and resize grip if scrollbars are enabled)
      // when we activate/deactivate the window.
      if (m_borderless || useScrollBarsHack()) {
        // From: https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-ncactivate
        // "If lparam is set to -1, DefWindowProc() does not repaint
        // the nonclient area to reflect the state change."
        //
        // Anyway that's not enough, if scrollbars are enabled,
        // DefWindowProc() will try to draw the whole frame + the
        // resize grip, a "simple" hack is setting the window as
        // temporarily hidden, and then restoring the style again.
        LONG oldStyle, style;
        oldStyle = style = GetWindowLong(m_hwnd, GWL_STYLE);

        // Remove these styles to avoid drawing the resize grip at the
        // bottom-right border of the window.
        style &= ~(WS_HSCROLL | WS_VSCROLL);
        if (m_borderless) {
          // Avoid drawing the old-looking frame of the window.
          style &= ~WS_THICKFRAME;
        }

        SetWindowLong(m_hwnd, GWL_STYLE, style);
        auto res = DefWindowProc(m_hwnd, msg, wparam, lparam);
        SetWindowLong(m_hwnd, GWL_STYLE, oldStyle);
        return res;
      }
      break;

    case WM_ACTIVATE:
      if (wparam == WA_ACTIVE || wparam == WA_CLICKACTIVE) {
        checkColorSpaceChange();
      }

      if (m_hpenctx) {
        if (auto sys = system()) {
          // Handle z-order of Wintab context
          auto& api = sys->wintabApi();
          api.overlap(m_hpenctx, (wparam == WA_ACTIVE || wparam == WA_CLICKACTIVE) ? TRUE : FALSE);
        }
      }
      break;

    case WM_PAINT:
      if (m_isCreated) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        // If BeginPaint() returns null, it's highly probable that we
        // reached the limit of 10000 GDI objects (so there might be
        // some GDI leaks in the program).
        ASSERT(hdc);

        onPaint(hdc);
        EndPaint(m_hwnd, &ps);
        return true;
      }
      break;

    case WM_SIZING: {
      RECT* rect = reinterpret_cast<RECT*>(lparam);

      // Get the border needed on each side between the window rect
      // and the client area.
      int dx, dy;
      {
        RECT frame, client;
        GetWindowRect(m_hwnd, &frame);
        GetClientRect(m_hwnd, &client);
        dx = (frame.right - frame.left) - (client.right - client.left);
        dy = (frame.bottom - frame.top) - (client.bottom - client.top);
      }

      // We align the client area size to the m_scale.
      int w = std::max<int>(rect->right - rect->left, 0) - dx;
      int h = std::max<int>(rect->bottom - rect->top, 0) - dy;
      w = std::max<int>(w - (w % m_scale), 8 * m_scale) + dx;
      h = std::max<int>(h - (h % m_scale), 8 * m_scale) + dy;

      switch (wparam) {
        case WMSZ_LEFT:  rect->left = rect->right - w; break;
        case WMSZ_RIGHT: rect->right = rect->left + w; break;
        case WMSZ_TOP:   rect->top = rect->bottom - h; break;
        case WMSZ_TOPLEFT:
          rect->left = rect->right - w;
          rect->top = rect->bottom - h;
          break;
        case WMSZ_TOPRIGHT:
          rect->top = rect->bottom - h;
          rect->right = rect->left + w;
          break;
        case WMSZ_BOTTOM: rect->bottom = rect->top + h; break;
        case WMSZ_BOTTOMLEFT:
          rect->left = rect->right - w;
          rect->bottom = rect->top + h;
          break;
        case WMSZ_BOTTOMRIGHT:
          rect->right = rect->left + w;
          rect->bottom = rect->top + h;
          break;
      }
      break;
    }

    case WM_SIZE:
      if (m_isCreated) {
        gfx::Size newSize(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

        if (newSize.w > 0 && newSize.h > 0 && m_clientSize != newSize) {
          m_clientSize = newSize;
          onResize(m_clientSize);
          InvalidateRect(m_hwnd, nullptr, FALSE);
        }

        WINDOWPLACEMENT pl;
        pl.length = sizeof(pl);
        if (GetWindowPlacement(m_hwnd, &pl) && !m_fullscreen) {
          m_restoredFrame = gfx::Rect(pl.rcNormalPosition.left,
                                      pl.rcNormalPosition.top,
                                      pl.rcNormalPosition.right - pl.rcNormalPosition.left,
                                      pl.rcNormalPosition.bottom - pl.rcNormalPosition.top);
        }
      }
      break;

    case WM_ENTERSIZEMOVE: onStartResizing(); break;

    case WM_EXITSIZEMOVE:
      checkColorSpaceChange();
      onEndResizing();
      break;

    case WM_SETTINGCHANGE:
      checkDarkModeChange();
      break;

      // Mouse and Trackpad Messages

    case WM_MOUSEMOVE: {
      Event ev;
      mouseEvent(lparam, ev);

      // Filter spurious mouse move messages out. Sometimes we receive
      // periodic (e.g. each 2 seconds) WM_MOUSEMOVE messages in the
      // same mouse position even when the mouse is not moved. This is
      // specially problematic when a mouse and a stylus are plugged
      // in, because there are random jumps between the pen position
      // and the mouse position (receiving a random WM_MOUSEMOVE from
      // the mouse position in the middle of a flow of
      // WM_POINTERUPDATE messages from the pen).
      {
        static HWND lastHwnd = nullptr;
        static gfx::Point lastPoint;
        if (lastHwnd == m_hwnd && lastPoint == ev.position()) {
          MOUSE_TRACE("SAME MOUSEMOVE xy=%d,%d\n", ev.position().x, ev.position().y);
          break;
        }
        lastHwnd = m_hwnd;
        lastPoint = ev.position();
      }

      MOUSE_TRACE("MOUSEMOVE xy=%d,%d\n", ev.position().x, ev.position().y);

      if (m_ignoreRandomMouseEvents > 0) {
        MOUSE_TRACE(" - IGNORED (random event)\n");
        --m_ignoreRandomMouseEvents;
        break;
      }

      handleMouseMove(ev);
      break;
    }

    case WM_NCLBUTTONDOWN:
      if (m_borderless) {
        // With custom frames, simulate that we always clicked the
        // client area. So in this way Windows doesn't paint the
        // default Minimize/Maximize/Close buttons.
        POINT pos = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
        ScreenToClient(m_hwnd, &pos);

        LPARAM rellparam = MAKELPARAM(pos.x, pos.y);
        return SendMessage(m_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, rellparam);
      }
      break;

    case WM_NCMOUSEMOVE:
    case WM_MOUSELEAVE:
      // For regular windows (with the default system frame), we
      // generate the MouseLeave message when the mouse leaves the
      // client area.
      if (m_hasMouse && !m_borderless) {
        handleMouseLeave();
      }
      break;

    case WM_NCMOUSELEAVE:
      // When the window doesn't have borders (m_borderless), we send
      // the MouseLeave event when the mouse leaves the non-client
      // area. This is required when handleHitTest() function is
      // specified, because when the hit test is != HTCLIENT the
      // WM_MOUSELEAVE is sent by Windows (and we don't want to
      // generate the MouseLeave in that case, only when the mouse
      // leaves the custom frame of the window, that is in this
      // WM_NCMOUSELEAVE message).
      if (m_hasMouse && m_borderless) {
        handleMouseLeave();
      }
      break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN: {
      Event ev;
      mouseEvent(lparam, ev);
      ev.setType(Event::MouseDown);
      ev.setButton(msg == WM_LBUTTONDOWN                                    ? Event::LeftButton :
                   msg == WM_RBUTTONDOWN                                    ? Event::RightButton :
                   msg == WM_MBUTTONDOWN                                    ? Event::MiddleButton :
                   msg == WM_XBUTTONDOWN && GET_XBUTTON_WPARAM(wparam) == 1 ? Event::X1Button :
                   msg == WM_XBUTTONDOWN && GET_XBUTTON_WPARAM(wparam) == 2 ? Event::X2Button :
                                                                              Event::NoneButton);

      if (m_pointerType != PointerType::Unknown) {
        ev.setPointerType(m_pointerType);
        ev.setPressure(m_pressure);
      }

      MOUSE_TRACE("BUTTONDOWN xy=%d,%d button=%d pointerType=%d\n",
                  ev.position().x,
                  ev.position().y,
                  ev.button(),
                  (int)m_pointerType);

      if (tabletAPI() == TabletAPI::WintabPackets && same_mouse_event(ev, m_lastWintabEvent)) {
        MOUSE_TRACE(" - IGNORED (WinTab)\n");
      }
      else {
        queueEvent(ev);
        m_lastWintabEvent.setType(Event::None);
      }
      break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP: {
      Event ev;
      mouseEvent(lparam, ev);
      ev.setType(Event::MouseUp);
      ev.setButton(msg == WM_LBUTTONUP                                    ? Event::LeftButton :
                   msg == WM_RBUTTONUP                                    ? Event::RightButton :
                   msg == WM_MBUTTONUP                                    ? Event::MiddleButton :
                   msg == WM_XBUTTONUP && GET_XBUTTON_WPARAM(wparam) == 1 ? Event::X1Button :
                   msg == WM_XBUTTONUP && GET_XBUTTON_WPARAM(wparam) == 2 ? Event::X2Button :
                                                                            Event::NoneButton);

      if (m_pointerType != PointerType::Unknown) {
        ev.setPointerType(m_pointerType);
        ev.setPressure(m_pressure);
      }

      MOUSE_TRACE("BUTTONUP xy=%d,%d button=%d\n", ev.position().x, ev.position().y, ev.button());

      if (tabletAPI() == TabletAPI::WintabPackets && same_mouse_event(ev, m_lastWintabEvent)) {
        MOUSE_TRACE(" - IGNORED (WinTab)\n");
      }
      else {
        queueEvent(ev);
        m_lastWintabEvent.setType(Event::None);
      }

      // Avoid popup menu for scrollbars
      if (msg == WM_RBUTTONUP)
        return 0;

      break;
    }

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK: {
      Event ev;
      mouseEvent(lparam, ev);
      ev.setType(Event::MouseDoubleClick);
      ev.setButton(msg == WM_LBUTTONDBLCLK ? Event::LeftButton :
                   msg == WM_RBUTTONDBLCLK ? Event::RightButton :
                   msg == WM_MBUTTONDBLCLK ? Event::MiddleButton :
                   msg == WM_XBUTTONDBLCLK && GET_XBUTTON_WPARAM(wparam) == 1 ? Event::X1Button :
                   msg == WM_XBUTTONDBLCLK && GET_XBUTTON_WPARAM(wparam) == 2 ? Event::X2Button :
                                                                                Event::NoneButton);

      if (m_pointerType != PointerType::Unknown) {
        ev.setPointerType(m_pointerType);
        ev.setPressure(m_pressure);
      }

      MOUSE_TRACE("BUTTONDBLCLK xy=%d,%d button=%d\n",
                  ev.position().x,
                  ev.position().y,
                  ev.button());
      queueEvent(ev);
      break;
    }

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
      POINT pos = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
      ScreenToClient(m_hwnd, &pos);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setPosition(gfx::Point(pos.x, pos.y) / m_scale);

      int z = GET_WHEEL_DELTA_WPARAM(wparam);
      if (ABS(z) >= WHEEL_DELTA)
        z /= WHEEL_DELTA;
      else {
        // TODO use floating point numbers or something similar
        //      (so we could use: z /= double(WHEEL_DELTA))
        z = SGN(z);
      }

      gfx::Point delta((msg == WM_MOUSEHWHEEL ? z : 0), (msg == WM_MOUSEWHEEL ? -z : 0));
      ev.setWheelDelta(delta);
      queueEvent(ev);

      MOUSE_TRACE("MOUSEWHEEL xy=%d,%d delta=%d,%d\n",
                  ev.position().x,
                  ev.position().y,
                  ev.wheelDelta().x,
                  ev.wheelDelta().y);
      break;
    }

    case WM_HSCROLL:
    case WM_VSCROLL: {
      POINT pos;
      GetCursorPos(&pos);
      ScreenToClient(m_hwnd, &pos);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setPosition(gfx::Point(pos.x, pos.y) / m_scale);

      int bar = (msg == WM_HSCROLL ? SB_HORZ : SB_VERT);
      int z = GetScrollPos(m_hwnd, bar);

      switch (LOWORD(wparam)) {
        case SB_LEFT:
        case SB_LINELEFT:  --z; break;
        case SB_PAGELEFT:  z -= 2; break;
        case SB_RIGHT:
        case SB_LINERIGHT: ++z; break;
        case SB_PAGERIGHT: z += 2; break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
        case SB_ENDSCROLL:
          // Do nothing
          break;
      }

      gfx::Point delta((msg == WM_HSCROLL ? (z - 50) : 0), (msg == WM_VSCROLL ? (z - 50) : 0));
      ev.setWheelDelta(delta);

      SetScrollPos(m_hwnd, bar, 50, FALSE);
      queueEvent(ev);

      MOUSE_TRACE("HVSCROLL xy=%d,%d delta=%d,%d\n",
                  ev.position().x,
                  ev.position().y,
                  ev.wheelDelta().x,
                  ev.wheelDelta().y);
      break;
    }

      // Pointer API (since Windows 8.0)

    case WM_POINTERCAPTURECHANGED: {
      MOUSE_TRACE("POINTERCAPTURECHANGED\n");
      releaseMouse();
      break;
    }

    case WM_POINTERENTER: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      MOUSE_TRACE("POINTERENTER id=%d xy=%d,%d\n", pi.pointerId, ev.position().x, ev.position().y);

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.AddPointerInteractionContext) {
          winApi.AddPointerInteractionContext(m_ictx, pi.pointerId);

          if (m_touch && pi.pointerType == PT_TOUCH && !m_touch->asMouse) {
            ++m_touch->fingers;
            TOUCH_TRACE("POINTERENTER fingers=%d\n", m_touch->fingers);
            if (m_touch->fingers == 1) {
              waitTimerToConvertFingerAsMouseMovement();
            }
            else if (m_touch->canBeMouse && m_touch->fingers >= 2) {
              delegateFingerToInteractionContext();
            }
          }
        }
      }

      if (!m_hasMouse) {
        m_hasMouse = true;

        ev.setType(Event::MouseEnter);
        queueEvent(ev);

        system()->_setInternalMousePosition(ev);
        MOUSE_TRACE("-> Event::MouseEnter\n");
      }
      return 0;
    }

    case WM_POINTERLEAVE: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      MOUSE_TRACE("POINTERLEAVE id=%d\n", pi.pointerId);

      // After releasing a finger a WM_MOUSEMOVE event in the trackpad
      // position is generated, we'll ignore that message.
      if (m_touch)
        m_ignoreRandomMouseEvents = 1;
      else
        m_ignoreRandomMouseEvents = 0;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.RemovePointerInteractionContext) {
          winApi.RemovePointerInteractionContext(m_ictx, pi.pointerId);

          if (m_touch && pi.pointerType == PT_TOUCH) {
            if (m_touch->fingers > 0)
              --m_touch->fingers;
            TOUCH_TRACE("POINTERLEAVE fingers=%d\n", m_touch->fingers);
            if (m_touch->fingers == 0) {
              if (m_touch->canBeMouse)
                sendDelayedTouchEvents();
              else
                clearDelayedTouchEvents();
              killTouchTimer();
              m_touch->asMouse = false;
            }
          }
        }
      }

      system()->_clearInternalMousePosition();

#if 0 // Don't generate MouseLeave from pen/touch messages
      // TODO we should generate this message, but after this touch
      //      messages don't work anymore, so we have to fix that problem.
      if (m_hasMouse) {
        m_hasMouse = false;

        ev.setType(Event::MouseLeave);
        queueEvent(ev);

        MOUSE_TRACE("-> Event::MouseLeave\n");
        return 0;
      }
#endif
      break;
    }

    case WM_POINTERDOWN: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.ProcessPointerFramesInteractionContext) {
          winApi.ProcessPointerFramesInteractionContext(m_ictx, 1, 1, &pi);
          if (!m_touch && pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      handlePointerButtonChange(ev, pi);

      MOUSE_TRACE("POINTERDOWN id=%d xy=%d,%d button=%d\n",
                  pi.pointerId,
                  ev.position().x,
                  ev.position().y,
                  ev.button());
      return 0;
    }

    case WM_POINTERUP: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.ProcessPointerFramesInteractionContext) {
          winApi.ProcessPointerFramesInteractionContext(m_ictx, 1, 1, &pi);
          if (!m_touch && pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      handlePointerButtonChange(ev, pi);

      MOUSE_TRACE("POINTERUP id=%d xy=%d,%d button=%d\n",
                  pi.pointerId,
                  ev.position().x,
                  ev.position().y,
                  ev.button());
      return 0;
    }

    case WM_POINTERUPDATE: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      // See the comment for m_ignoreRandomMouseEvents variable, and
      // why here is = 2.
      m_ignoreRandomMouseEvents = 2;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.ProcessPointerFramesInteractionContext) {
          winApi.ProcessPointerFramesInteractionContext(m_ictx, 1, 1, &pi);
          if (!m_touch && pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      if (!m_hasMouse) {
        m_hasMouse = true;

        ev.setType(Event::MouseEnter);
        queueEvent(ev);

        MOUSE_TRACE("-> Event::MouseEnter\n");
      }

      ev.setType(Event::MouseMove);

      if (m_touch && pi.pointerType == PT_TOUCH) {
        TOUCH_TRACE("POINTERUPDATE canBeMouse=%d asMouse=%d\n",
                    m_touch->canBeMouse,
                    m_touch->asMouse);
        if (!m_touch->asMouse) {
          if (m_touch->canBeMouse)
            m_touch->delayedEvents.push_back(ev);
          else
            return 0;
        }
        else
          queueEvent(ev);
      }
      else
        queueEvent(ev);

      handlePointerButtonChange(ev, pi);

      MOUSE_TRACE("POINTERUPDATE id=%d xy=%d,%d\n", pi.pointerId, ev.position().x, ev.position().y);
      return 0;
    }

    case WM_POINTERWHEEL:
    case WM_POINTERHWHEEL: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      ev.setType(Event::MouseWheel);

      int z = GET_WHEEL_DELTA_WPARAM(wparam);
      if (ABS(z) >= WHEEL_DELTA)
        z /= WHEEL_DELTA;
      else {
        // TODO use floating point numbers or something similar
        //      (so we could use: z /= double(WHEEL_DELTA))
        z = SGN(z);
      }

      gfx::Point delta((msg == WM_POINTERHWHEEL ? z : 0), (msg == WM_POINTERWHEEL ? -z : 0));
      ev.setWheelDelta(delta);
      queueEvent(ev);

      MOUSE_TRACE("POINTERWHEEL xy=%d,%d delta=%d,%d\n",
                  ev.position().x,
                  ev.position().y,
                  ev.wheelDelta().x,
                  ev.wheelDelta().y);

      return 0;
    }

    case WM_TIMER:
      TOUCH_TRACE("TIMER %d\n", wparam);
      if (m_touch && m_touch->timerID == wparam) {
        killTouchTimer();

        if (!m_touch->asMouse && m_touch->canBeMouse && m_touch->fingers == 1) {
          TOUCH_TRACE("-> finger as mouse, sent %d events\n", m_touch->delayedEvents.size());

          convertFingerAsMouseMovement();
        }
        else {
          delegateFingerToInteractionContext();
        }
      }
      break;

      // Keyboard Messages

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:    {
      int vk = wparam;
      int scancode = (lparam >> 16) & 0xff;
      bool sendMsg = true;
      const KeyScancode ourScancode = win32vk_to_scancode(vk);

      // We only create one KeyDown event for modifiers. Bit 30
      // indicates the previous state of the key, if the modifier was
      // already pressed don't generate the event.
      if ((ourScancode >= kKeyFirstModifierScancode) && (lparam & (1 << 30)))
        return 0;

      Event ev;
      ev.setType(Event::KeyDown);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setScancode(ourScancode);
      ev.setUnicodeChar(0);
      ev.setRepeat(lparam & (1 << 30) ? 1 : 0);

      KEY_TRACE("KEYDOWN vk=%d scancode=%d->%d modifiers=%d\n",
                vk,
                scancode,
                ev.scancode(),
                ev.modifiers());

      {
        VkToUnicode tu;
        if (tu) {
          tu.toUnicode(vk, scancode);
          if (tu.isDeadKey()) {
            ev.setDeadKey(true);
            ev.setUnicodeChar(tu[0]);
            if (!m_translateDeadKeys)
              tu.toUnicode(vk, scancode); // Call again to remove dead-key
          }
          else if (tu.size() > 0) {
            sendMsg = false;
            for (int chr : tu) {
              ev.setUnicodeChar(chr);
              queueEvent(ev);

              KEY_TRACE(" -> queued unicode char=%d <%c>\n",
                        ev.unicodeChar(),
                        ev.unicodeChar() ? ev.unicodeChar() : ' ');
            }
          }
        }
      }

      if (sendMsg) {
        queueEvent(ev);
        KEY_TRACE(" -> queued unicode char=%d <%c>\n",
                  ev.unicodeChar(),
                  ev.unicodeChar() ? ev.unicodeChar() : ' ');
      }

      return 0;
    }

    case WM_SYSKEYUP:
    case WM_KEYUP:    {
      Event ev;
      ev.setType(Event::KeyUp);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setScancode(win32vk_to_scancode(wparam));
      ev.setUnicodeChar(0);
      ev.setRepeat(lparam & (1 << 30) ? 0 : 1);
      queueEvent(ev);

      // TODO If we use native menus, this message should be given
      // to the DefWindowProc() in some cases (e.g. F10 or Alt keys)
      return 0;
    }

    case WM_MENUCHAR:
      // Avoid playing a sound when Alt+key is pressed and it's not in a native menu
      return MAKELONG(0, MNC_CLOSE);

    case WM_DROPFILES: {
      HDROP hdrop = (HDROP)(wparam);
      base::paths files;

      int count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
      for (int index = 0; index < count; ++index) {
        int length = DragQueryFile(hdrop, index, NULL, 0);
        if (length > 0) {
          std::vector<TCHAR> str(length + 1);
          DragQueryFile(hdrop, index, &str[0], str.size());
          files.push_back(base::to_utf8(&str[0]));
        }
      }

      Event ev;
      ev.setType(Event::DropFiles);
      ev.setFiles(files);
      POINT pos;
      if (DragQueryPoint(hdrop, &pos))
        ev.setPosition(gfx::Point(pos.x, pos.y));

      DragFinish(hdrop);

      queueEvent(ev);
      break;
    }

    case WM_NCCALCSIZE: {
      if (wparam) {
        if (m_borderless) {
#if 0 // TODO this is not working yet because the taskbar is not
      //      visible (in some way Windows is still hidden the
      //      taskbar), we've fixed this in WM_WINDOWPOSCHANGED
          if (isMaximized() && !m_fullscreen) {
            // As a maximized window without WS_CAPTION | WS_THICKFRAME
            // styles is seen as a full screen window, Windows gives us the
            // full screen area (instead of the workarea). This is a hack
            // to avoid that and just give us the workarea for our custom
            // frame/client area.
            gfx::Rect wa = screen()->workarea();

            NCCALCSIZE_PARAMS* cs = reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);
            cs->rgrc[0].left = wa.x;
            cs->rgrc[0].top = wa.y;
            cs->rgrc[0].right = wa.x2();
            cs->rgrc[0].bottom = wa.y2();
          }
#endif

          // Not using DefProcWindow() here will avoid showing a
          // native frame around the our custom frame.
          //
          // The "WVR_REDRAW" is used to redraw the whole client area
          // (not reusing old regions after the resize operation).
          return WVR_REDRAW;
        }

        if (useScrollBarsHack()) {
          // Scrollbars must be enabled and visible to get trackpad
          // events of old drivers. So we cannot use ShowScrollBar() to
          // hide them. This is a simple (maybe not so elegant)
          // solution: Expand the client area to we overlap the
          // scrollbars. In this way they are not visible, but we still
          // get their messages.
          NCCALCSIZE_PARAMS* cs = reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);

          auto& winApi = system()->winApi();
          if (winApi.GetWindowDpiAwarenessContext) {
            UINT dpi = winApi.GetDpiForWindow(m_hwnd);
            cs->rgrc[0].right += winApi.GetSystemMetricsForDpi(SM_CYVSCROLL, dpi);
            cs->rgrc[0].bottom += winApi.GetSystemMetricsForDpi(SM_CYHSCROLL, dpi);
          } else {
            cs->rgrc[0].right += GetSystemMetrics(SM_CYVSCROLL);
            cs->rgrc[0].bottom += GetSystemMetrics(SM_CYHSCROLL);
          }
        }
      }
      break;
    }

    case WM_NCHITTEST: {
      gfx::Point pt(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

      // Custom handler for WM_NCHITTEST when the mouse is inside the
      // windows area.
      if (this->handleHitTest) {
        POINT pos = { pt.x, pt.y };
        ScreenToClient(m_hwnd, &pos);
        gfx::Point relPt(pos.x, pos.y);
        relPt /= m_scale;

        Event ev;
        ev.setModifiers(get_modifiers_from_last_win32_message());
        ev.setPosition(relPt);
        handleMouseMove(ev);

        // Convert os::Hit values to Win32 HT* values
        const int i = static_cast<int>(this->handleHitTest(this, relPt));
        return (i >= 0 && i < hit2hittest_entries ? static_cast<LRESULT>(hit2hittest[i]) :
                                                    HTNOWHERE);
      }

      LRESULT result = DefWindowProc(m_hwnd, msg, wparam, lparam);

      ABS_CLIENT_RC(rc);
      gfx::Rect area(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);

      // We ignore scrollbars so if the mouse is above them, we return
      // as it's in the window client or resize area. (Remember that
      // we have scroll bars are enabled and visible to receive
      // trackpad messages only.)
      if (result == HTHSCROLL) {
        result = (area.contains(pt) ? HTCLIENT : HTBOTTOM);
      }
      else if (result == HTVSCROLL) {
        result = (area.contains(pt) ? HTCLIENT : HTRIGHT);
      }
      // Filter the resize grip area of the bottom-right corner, which
      // has the size of the scrollbars and we don't want to use that
      // area to resize the window.
      else if (result == HTBOTTOMRIGHT) {
        if (area.contains(pt))
          result = HTCLIENT;
      }

      return result;
    }

      // Wintab API Messages

    case WT_PROXIMITY: {
      HCTX ctx = (HCTX)wparam;

      // This can happen when we switch from TabletAPI::Wintab to
      // TabletAPI::WintabPackets mode.
      if (m_hpenctx != ctx)
        break;

      bool entering_ctx = (LOWORD(lparam) ? true : false);
      if (entering_ctx && ctx) {
        auto& api = system()->wintabApi();

        // Get the cursor from the proximity packet
        PACKET packet;
        if (api.packets(ctx, 1, &packet) == 1) {
          m_pointerType = wt_packet_pkcursor_to_pointer_type(packet.pkCursor);
        }
      }
      else {
        m_pointerType = PointerType::Unknown;
      }

      MOUSE_TRACE("WT_PROXIMITY entering=%d pointerType=%d\n", entering_ctx, (int)m_pointerType);

      // Reset last event
      m_lastWintabEvent.setType(Event::None);
      break;
    }

    case WT_PACKET: {
      auto& api = system()->wintabApi();
      HCTX ctx = (HCTX)lparam;
      if (m_packets.size() < api.packetQueueSize())
        m_packets.resize(api.packetQueueSize());

      int n = api.packets(ctx, m_packets.size(), &m_packets[0]);
      MOUSE_TRACE("WT_PACKET packets=%d\n", n);

      // It looks like that we can process the whole queue with
      // WTPacketsGet() but we will receive one WT_PACKET for each
      // packet anyway (even when the queue is empty).
      if (n == 0)
        break;

      m_pointerType = PointerType::Unknown;

      Event ev;
      ev.setModifiers(get_modifiers_from_last_win32_message());

      for (int i = 0; i < n; ++i) {
        const PACKET& packet = m_packets[i];

        if (api.minPressure() < api.maxPressure()) {
          m_pressure = float(packet.pkNormalPressure - api.minPressure()) /
                       float(api.maxPressure() - api.minPressure());
        }
        else {
          m_pressure = 0.0f;
        }
        m_pointerType = wt_packet_pkcursor_to_pointer_type(packet.pkCursor);

        if (tabletAPI() == TabletAPI::WintabPackets) {
          POINT pos = { packet.pkX,
                        // Wintab API uses lower-left corner as the origin
                        (api.outBounds().h - 1) - packet.pkY };
          ScreenToClient(m_hwnd, &pos);

          ev.setPosition(gfx::Point(pos.x, pos.y) / m_scale);
          ev.setPointerType(m_pointerType);
          ev.setPressure(m_pressure);

          // Get mouse button and even type (mouse move/down/up/double-click)
          Event::Type evType;
          Event::MouseButton mouseButton;
          api.mapCursorButton(packet.pkCursor,
                              HIWORD(packet.pkButtons), // Logical button
                              LOWORD(packet.pkButtons), // Relative button flag (down/up)
                              evType,
                              mouseButton);

          ev.setType(evType);

          // Do not put the mouse button in mouse move events (so they
          // match in WM_MOUSEMOVE and we can ignore duplicated events)
          if (evType != Event::MouseMove)
            ev.setButton(mouseButton);

          MOUSE_TRACE(
            "  [%d] evType=%d xy=%d,%d pressure=%.4f evButton=%d pkCursor=%d pointerType=%d\n",
            i,
            ev.type(),
            ev.position().x,
            ev.position().y,
            m_pressure,
            (int)ev.button(),
            packet.pkCursor,
            (int)m_pointerType);

          if (evType != Event::None) {
            queueEvent(ev);

            // To avoid processing two times the last generated event in
            // WM_MOUSEMOVE/WM_LBUTTONDOWN/UP
            m_lastWintabEvent = ev;

            // Don't store a reference to the window (without this
            // windows cannot be closed after storing a reference).
            m_lastWintabEvent.setWindow(nullptr);
          }
        }
      }
      break;
    }

    case WT_INFOCHANGE: {
      MOUSE_TRACE("WT_INFOCHANGE tablet API %d\n", int(tabletAPI()));

      if (m_hpenctx) {
        closeWintabCtx();

        // Wacom examples show that we have to wait a second so the
        // driver can identify the attached tablets.
        base::this_thread::sleep_for(1.0);
      }

      openWintabCtx();
      break;
    }
  }

  LRESULT result = FALSE;
  if (handle_dde_messages(m_hwnd, msg, wparam, lparam, result))
    return result;

  return DefWindowProc(m_hwnd, msg, wparam, lparam);
}

void WindowWin::mouseEvent(LPARAM lparam, Event& ev)
{
  ev.setModifiers(get_modifiers_from_last_win32_message());
  ev.setPosition(gfx::Point(GET_X_LPARAM(lparam) / m_scale, GET_Y_LPARAM(lparam) / m_scale));
}

bool WindowWin::pointerEvent(WPARAM wparam, Event& ev, POINTER_INFO& pi)
{
  if (!m_usePointerApi)
    return false;

  auto sys = system();
  if (!sys)
    return false;

  auto& winApi = sys->winApi();
  if (!winApi.GetPointerInfo(GET_POINTERID_WPARAM(wparam), &pi))
    return false;

  ABS_CLIENT_RC(rc);

  ev.setModifiers(get_modifiers_from_last_win32_message());
  ev.setPosition(gfx::Point((pi.ptPixelLocation.x - rc.left) / m_scale,
                            (pi.ptPixelLocation.y - rc.top) / m_scale));

  // Calling SetCursorPos() when we receive a pointer event from the
  // stylus can fix issues live streaming with OBS and a stylus, where
  // the captured mouse position is always obtained through
  // GetCursorPos() but we are in other position with the stylus.
  if (sys->tabletOptions().setCursorFix) {
    SetCursorPos(pi.ptPixelLocation.x, pi.ptPixelLocation.y);
  }

  switch (pi.pointerType) {
    case PT_MOUSE: {
      MOUSE_TRACE("pi.pointerType PT_MOUSE\n");
      ev.setPointerType(PointerType::Mouse);

      // If we use EnableMouseInPointer(true), events from Wacom
      // stylus came as PT_MOUSE instead of PT_PEN with eraser
      // flag. This is just insane, EnableMouseInPointer(true) is not
      // an option at the moment if we want proper support for Wacom
      // events.
      break;
    }
    case PT_TOUCH: {
      MOUSE_TRACE("pi.pointerType PT_TOUCH\n");
      ev.setPointerType(PointerType::Touch);
      break;
    }
    case PT_TOUCHPAD: {
      MOUSE_TRACE("pi.pointerType PT_TOUCHPAD\n");
      ev.setPointerType(PointerType::Touchpad);
      break;
    }
    case PT_PEN: {
      MOUSE_TRACE("pi.pointerType PT_PEN\n");
      ev.setPointerType(PointerType::Pen);

      POINTER_PEN_INFO ppi;
      if (winApi.GetPointerPenInfo(pi.pointerId, &ppi)) {
        MOUSE_TRACE(" - ppi.penFlags = %d\n", ppi.penFlags);
        if (ppi.penFlags & PEN_FLAG_ERASER)
          ev.setPointerType(PointerType::Eraser);

        // Add pressure information
        ev.setPressure(std::clamp(float(ppi.pressure) / 1024.0f, 0.0f, 1.0f));
      }
      break;
    }
  }
  return true;
}

void WindowWin::handleMouseMove(Event& ev)
{
  if (!m_hasMouse) {
    m_hasMouse = true;

    ev.setType(Event::MouseEnter);
    queueEvent(ev);

    MOUSE_TRACE("-> Event::MouseEnter\n");

    // Track mouse to receive WM_MOUSELEAVE and WM_NCMOUSELEAVE message.
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = m_hwnd;
    _TrackMouseEvent(&tme);
  }

  if (m_pointerType != PointerType::Unknown) {
    ev.setPointerType(m_pointerType);
    ev.setPressure(m_pressure);
  }

  ev.setType(Event::MouseMove);

  if (tabletAPI() == TabletAPI::WintabPackets && same_mouse_event(ev, m_lastWintabEvent)) {
    MOUSE_TRACE(" - IGNORED (WinTab)\n");
  }
  else {
    queueEvent(ev);
    m_lastWintabEvent.setType(Event::None);

    system()->_setInternalMousePosition(ev);
  }
}

void WindowWin::handleMouseLeave()
{
  ASSERT(m_hasMouse);
  m_hasMouse = false;

  Event ev;
  ev.setType(Event::MouseLeave);
  ev.setModifiers(get_modifiers_from_last_win32_message());
  queueEvent(ev);

  system()->_clearInternalMousePosition();
  MOUSE_TRACE("-> Event::MouseLeave\n");
}

void WindowWin::handlePointerButtonChange(Event& ev, POINTER_INFO& pi)
{
  if (pi.ButtonChangeType == POINTER_CHANGE_NONE) {
#if OS_USE_POINTER_API_FOR_MOUSE
    // Reset the counter of pointer down for the emulated double-click
    if (m_emulateDoubleClick)
      m_pointerDownCount = 0;
#endif

    // Update the internal last mouse position because if a pointer
    // button wasn't change, the position might have changed anyway
    // (i.e. we're in a WM_POINTERUPDATE event).
    system()->_setInternalMousePosition(ev);
    return;
  }

  Event::MouseButton button = Event::NoneButton;
  bool down = false;

  switch (pi.ButtonChangeType) {
    case POINTER_CHANGE_FIRSTBUTTON_DOWN:  down = true;
    case POINTER_CHANGE_FIRSTBUTTON_UP:    button = Event::LeftButton; break;
    case POINTER_CHANGE_SECONDBUTTON_DOWN: down = true;
    case POINTER_CHANGE_SECONDBUTTON_UP:   button = Event::RightButton; break;
    case POINTER_CHANGE_THIRDBUTTON_DOWN:  down = true;
    case POINTER_CHANGE_THIRDBUTTON_UP:    button = Event::MiddleButton; break;
    case POINTER_CHANGE_FOURTHBUTTON_DOWN: down = true;
    case POINTER_CHANGE_FOURTHBUTTON_UP:   button = Event::X1Button; break;
    case POINTER_CHANGE_FIFTHBUTTON_DOWN:  down = true;
    case POINTER_CHANGE_FIFTHBUTTON_UP:    button = Event::X2Button; break;
  }

  if (button == Event::NoneButton)
    return;

  ev.setType(down ? Event::MouseDown : Event::MouseUp);
  ev.setButton(button);

#if OS_USE_POINTER_API_FOR_MOUSE
  if (down && m_emulateDoubleClick) {
    if (button != m_lastPointerDownButton)
      m_pointerDownCount = 0;

    ++m_pointerDownCount;

    base::tick_t curTime = base::current_tick();
    if ((m_pointerDownCount == 2) && (curTime - m_lastPointerDownTime) <= m_doubleClickMsecs) {
      ev.setType(Event::MouseDoubleClick);
      m_pointerDownCount = 0;
    }

    m_lastPointerDownTime = curTime;
    m_lastPointerDownButton = button;
  }
#endif

  if (m_touch && pi.pointerType == PT_TOUCH) {
    if (!m_touch->asMouse) {
      if (m_touch->canBeMouse) {
        // TODO Review why the ui layer needs a Event::MouseMove event
        //      before ButtonDown/Up events.
        Event evMouseMove = ev;
        evMouseMove.setType(Event::MouseMove);
        m_touch->delayedEvents.push_back(evMouseMove);
        m_touch->delayedEvents.push_back(ev);
      }
      return;
    }
  }

  queueEvent(ev);
  system()->_setInternalMousePosition(ev);
}

void WindowWin::handleInteractionContextOutput(const INTERACTION_CONTEXT_OUTPUT* output)
{
  MOUSE_TRACE("%s (%d) xy=%.16g %.16g flags=%d type=%d\n",
              output->interactionId == INTERACTION_ID_MANIPULATION ? "INTERACTION_ID_MANIPULATION" :
              output->interactionId == INTERACTION_ID_TAP          ? "INTERACTION_ID_TAP" :
              output->interactionId == INTERACTION_ID_SECONDARY_TAP ?
                                                            "INTERACTION_ID_SECONDARY_TAP" :
              output->interactionId == INTERACTION_ID_HOLD ? "INTERACTION_ID_HOLD" :
                                                             "INTERACTION_ID_???",
              output->interactionId,
              output->x,
              output->y,
              output->interactionFlags,
              output->inputType);

  // We use the InteractionContext to interpret touch gestures only
  // and double tap with pen.
  if ((output->inputType == PT_TOUCH && (!m_touch || (!m_touch->asMouse && !m_touch->canBeMouse) ||
                                         (output->arguments.tap.count == 2))) ||
      (output->inputType == PT_PEN && output->interactionId == INTERACTION_ID_TAP &&
       output->arguments.tap.count == 2)) {
    ABS_CLIENT_RC(rc);

    gfx::Point pos(int((output->x - rc.left) / m_scale), int((output->y - rc.top) / m_scale));

    Event ev;
    // This is PT_PEN or PT_TOUCH
    ev.setPointerType(output->inputType == PT_PEN ? PointerType::Pen : PointerType::Touch);
    ev.setModifiers(get_modifiers_from_last_win32_message());
    ev.setPosition(pos);

    bool hadMouse = m_hasMouse;
    if (!m_hasMouse) {
      m_hasMouse = true;
      ev.setType(Event::MouseEnter);
      queueEvent(ev);
    }

    switch (output->interactionId) {
      case INTERACTION_ID_MANIPULATION: {
        MOUSE_TRACE(" - delta xy=%.16g %.16g scale=%.16g expansion=%.16g rotation=%.16g\n",
                    output->arguments.manipulation.delta.translationX,
                    output->arguments.manipulation.delta.translationY,
                    output->arguments.manipulation.delta.scale,
                    output->arguments.manipulation.delta.expansion,
                    output->arguments.manipulation.delta.rotation);

        // TODO we should not change the sign
        gfx::Point delta(-int(output->arguments.manipulation.delta.translationX) / m_scale,
                         -int(output->arguments.manipulation.delta.translationY) / m_scale);

        if (output->interactionFlags & INTERACTION_FLAG_BEGIN) {
          ev.setType(Event::MouseMove);
          queueEvent(ev);
        }

        ev.setType(Event::MouseWheel);
        ev.setWheelDelta(delta);
        ev.setPreciseWheel(true);
        queueEvent(ev);

        ev.setType(Event::TouchMagnify);
        ev.setMagnification(output->arguments.manipulation.delta.scale - 1.0);
        queueEvent(ev);
        break;
      }

      case INTERACTION_ID_TAP:
        MOUSE_TRACE(" - count=%d\n", output->arguments.tap.count);

        ev.setButton(Event::LeftButton);
        ev.setType(Event::MouseMove);
        queueEvent(ev);
        if (output->arguments.tap.count == 2) {
          ev.setType(Event::MouseDoubleClick);
          queueEvent(ev);
        }
        else {
          ev.setType(Event::MouseDown);
          queueEvent(ev);
          ev.setType(Event::MouseUp);
          queueEvent(ev);
        }
        break;

      case INTERACTION_ID_SECONDARY_TAP:
      case INTERACTION_ID_HOLD:
        ev.setButton(Event::RightButton);
        ev.setType(Event::MouseMove);
        queueEvent(ev);
        ev.setType(Event::MouseDown);
        queueEvent(ev);
        ev.setType(Event::MouseUp);
        queueEvent(ev);
        break;
    }
  }
}

void WindowWin::waitTimerToConvertFingerAsMouseMovement()
{
  ASSERT(m_touch);
  m_touch->canBeMouse = true;
  clearDelayedTouchEvents();
  SetTimer(m_hwnd, m_touch->timerID = 1, kFingerAsMouseTimeout, nullptr);
  TOUCH_TRACE(" - Set timer\n");
}

void WindowWin::convertFingerAsMouseMovement()
{
  ASSERT(m_touch);
  m_touch->asMouse = true;
  sendDelayedTouchEvents();
}

void WindowWin::delegateFingerToInteractionContext()
{
  ASSERT(m_touch);
  m_touch->canBeMouse = false;
  m_touch->asMouse = false;
  clearDelayedTouchEvents();
  if (m_touch->timerID > 0)
    killTouchTimer();
}

void WindowWin::sendDelayedTouchEvents()
{
  ASSERT(m_touch);
  for (auto& ev : m_touch->delayedEvents)
    queueEvent(ev);
  clearDelayedTouchEvents();
}

void WindowWin::clearDelayedTouchEvents()
{
  ASSERT(m_touch);
  m_touch->delayedEvents.clear();
}

void WindowWin::killTouchTimer()
{
  ASSERT(m_touch);
  if (m_touch->timerID > 0) {
    KillTimer(m_hwnd, m_touch->timerID);
    m_touch->timerID = 0;
    TOUCH_TRACE(" - Kill timer\n");
  }
}

void WindowWin::checkColorSpaceChange()
{
  os::ColorSpaceRef oldColorSpace = m_lastColorProfile;
  os::ColorSpaceRef newColorSpace = colorSpace();
  if (oldColorSpace != newColorSpace)
    onChangeColorSpace();
}

// We only support the Windows 11 dark mode, detecting it and using
// the correct (dark/light) title bar.
//
// A lot of information and discussion about this can be found here:
// https://github.com/microsoft/WindowsAppSDK/issues/41
void WindowWin::checkDarkModeChange()
{
  try {
    auto key = base::hkey::current_user().open(kPersonalizeKey, base::hkey::read);

    // We can check if "AppsUseLightTheme" option is 0 which means
    // that we have to use the dark theme. Not sure why this is not
    // automatic or the default behavior on Windows 11, or configured
    // through the manifest for Win32 apps.
    if (key.exists(kAppsUseLightThemeValue)) {
      BOOL value = (key.dword(kAppsUseLightThemeValue) == 0);
      DwmSetWindowAttribute(m_hwnd,
                            20, // DWMWA_USE_IMMERSIVE_DARK_MODE,
                            &value,
                            sizeof(BOOL));
    }
  }
  catch (const base::Win32Exception&) {
    // Probably the "Personalize" key doesn't exist because we are on
    // Windows 10, so we do nothing, which means: use the light
    // theme/current theme/title bar as it is.
  }
}

void WindowWin::openWintabCtx()
{
  auto sys = system();
  TabletOptions options = sys->tabletOptions();
  if (options.api == TabletAPI::Wintab || options.api == TabletAPI::WintabPackets) {
    // Attach Wacom context
    auto& api = sys->wintabApi();
    m_hpenctx = api.open(m_hwnd,
                         true); // We want to move the cursor with the pen in any case

    if (api.crashedBefore()) {
      options.api = TabletAPI::Default;
      sys->setTabletOptions(options);
    }
  }
}

void WindowWin::closeWintabCtx()
{
  if (!m_hpenctx)
    return;

  if (auto sys = system()) {
    auto& api = sys->wintabApi();
    api.close(m_hpenctx);
  }
  m_hpenctx = nullptr;
}

void WindowWin::notifyFullScreenStateToShell()
{
  base::ComPtr<ITaskbarList2> taskbar;
  HRESULT hr =
    CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbar));
  if (FAILED(hr))
    return;

  // Useful to send the taskbar at the bottom when the window is set
  // as fullscreen.
  taskbar->MarkFullscreenWindow(m_hwnd, m_fullscreen ? TRUE : FALSE);
}

TabletAPI WindowWin::tabletAPI() const
{
  if (auto sys = system())
    return sys->tabletOptions().api;
  else
    return TabletAPI::Default;
}

// static
void WindowWin::registerClass()
{
  HMODULE instance = GetModuleHandle(nullptr);
  const auto className = get_wnd_class_name();

  WNDCLASSEX wcex;
  if (GetClassInfoEx(instance, className.c_str(), &wcex))
    return; // Already registered

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_DBLCLKS;
  wcex.lpfnWndProc = &WindowWin::staticWndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = instance;
  wcex.hIcon = LoadIcon(instance, L"0");
  wcex.hCursor = NULL;
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME + 1);
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = className.c_str();
  wcex.hIconSm = nullptr;

  if (RegisterClassEx(&wcex) == 0)
    throw std::runtime_error("Error registering window class");
}

// static
HWND WindowWin::createHwnd(WindowWin* self, const WindowSpec& spec)
{
  int exStyle = WS_EX_ACCEPTFILES;
  int style = 0;
  if (spec.titled()) {
    if (!spec.parent())
      exStyle |= WS_EX_APPWINDOW;
    style |= WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
  }
  else {
    style |= WS_POPUP;
  }
  if (spec.minimizable()) {
    style |= WS_SYSMENU | WS_MINIMIZEBOX;
  }
  if (spec.resizable() ||
      // Without WS_THICKFRAME we get a white canvas for borderless
      // windows because we don't receive a WM_SIZE as we intercept
      // WM_NCCALCSIZE in this case.
      spec.borderless()) {
    style |= WS_THICKFRAME;
  }
  if (spec.maximizable()) {
    style |= WS_MAXIMIZEBOX;
  }
  if (spec.floating()) {
    exStyle |= WS_EX_TOOLWINDOW;
  }
  if (spec.transparent()) {
    exStyle |= WS_EX_LAYERED;
  }

  gfx::Rect rc;

  switch (spec.position()) {
    case WindowSpec::Position::Default:
      rc.x = CW_USEDEFAULT;
      rc.y = CW_USEDEFAULT;
      break;
    case WindowSpec::Position::Frame:       rc = spec.frame(); break;
    case WindowSpec::Position::ContentRect: rc = spec.contentRect(); break;
  }

  if (!spec.contentRect().isEmpty()) {
    rc.w = spec.contentRect().w;
    rc.h = spec.contentRect().h;
    RECT ncrc = { 0, 0, rc.w, rc.h };

    auto& winApi = system()->winApi();
    if (winApi.GetWindowDpiAwarenessContext) {
      UINT dpi = winApi.GetDpiForWindow(self->m_hwnd);
      winApi.AdjustWindowRectExForDpi(&ncrc,
                              style,
                              false, // Add a field to WindowSpec to add native menu bars
                              exStyle,
                              dpi);
    } else {
      AdjustWindowRectEx(&ncrc,
                        style,
                        false, // Add a field to WindowSpec to add native menu bars
                        exStyle);
    }

    if (rc.x != CW_USEDEFAULT)
      rc.x += ncrc.left;
    if (rc.y != CW_USEDEFAULT)
      rc.y += ncrc.top;
    rc.w = ncrc.right - ncrc.left;
    rc.h = ncrc.bottom - ncrc.top;
  }
  else if (!spec.frame().isEmpty()) {
    rc.w = spec.frame().w;
    rc.h = spec.frame().h;
  }
  else {
    rc.w = CW_USEDEFAULT;
    rc.h = CW_USEDEFAULT;
  }

  const auto className = get_wnd_class_name();
  HWND hwnd = CreateWindowEx(
    exStyle,
    className.c_str(),
    L"",
    style,
    rc.x,
    rc.y,
    rc.w,
    rc.h,
    (HWND)(spec.parent() ? static_cast<WindowWin*>(spec.parent())->nativeHandle() : nullptr),
    nullptr,
    GetModuleHandle(nullptr),
    reinterpret_cast<LPVOID>(self));
  if (!hwnd)
    return nullptr;

  return hwnd;
}

// static
LRESULT CALLBACK WindowWin::staticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  WindowWin* wnd = nullptr;

  if (msg == WM_CREATE) {
    wnd = reinterpret_cast<WindowWin*>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);

    if (wnd && wnd->m_hwnd == nullptr)
      wnd->m_hwnd = hwnd;
      
    // Enable scaling for titlebar with legacy per-monitor dpi awareness steeting
    auto& winApi = system()->winApi();
    if (winApi.GetWindowDpiAwarenessContext) {
      DPI_AWARENESS_CONTEXT dpiContext = winApi.GetWindowDpiAwarenessContext(hwnd);
      if (!winApi.AreDpiAwarenessContextsEqual(dpiContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        winApi.EnableNonClientDpiScaling(hwnd);
    }
  }
  else {
    wnd = reinterpret_cast<WindowWin*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // Check that the user data makes sense
    if (wnd && wnd->m_hwnd != hwnd)
      wnd = nullptr;
  }

  if (wnd) {
    ASSERT(wnd->m_hwnd == hwnd);
    return wnd->wndProc(msg, wparam, lparam);
  }
  else {
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

// static
void CALLBACK WindowWin::staticInteractionContextCallback(void* clientData,
                                                          const INTERACTION_CONTEXT_OUTPUT* output)
{
  WindowWin* self = reinterpret_cast<WindowWin*>(clientData);
  self->handleInteractionContextOutput(output);
}

// static
SystemWin* WindowWin::system()
{
  return static_cast<SystemWin*>(os::instance());
}

} // namespace os
