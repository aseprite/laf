// LAF OS Library
// Copyright (C) 2020-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/log.h"
#include "os/win/system.h"

#include "gfx/color.h"
#include "os/win/screen.h"

#include <limits>

#pragma push_macro("ERROR")
#undef ERROR

namespace os {

bool win_is_key_pressed(KeyScancode scancode);
int win_get_unicode_from_scancode(KeyScancode scancode);

static const gfx::Point kUnknownPos(std::numeric_limits<int>::min(),
                                    std::numeric_limits<int>::min());

class HBitmapPtr {
public:
  HBitmapPtr() : m_ptr(nullptr) { }
  ~HBitmapPtr() { reset(); }

  HBitmapPtr(const HBitmapPtr&) = delete;
  HBitmapPtr& operator=(const HBitmapPtr&) = delete;

  void reset(HBITMAP p = nullptr) {
    if (m_ptr)
      DeleteObject(m_ptr);
    m_ptr = p;
  }
  HBITMAP get() { return m_ptr; }
  operator bool() { return m_ptr != nullptr; }

private:
  HBITMAP m_ptr;
};

class CursorWin : public Cursor {
public:
  CursorWin(HCURSOR hcursor) : m_hcursor(hcursor) { }
  ~CursorWin() {
    // The m_hcursor can be nullptr if the cursor is completelly
    // transparent = equivalent to a Hidden cursor.
    if (m_hcursor) {
      DestroyIcon(m_hcursor);
    }
  }

  CursorWin(const CursorWin&) = delete;
  CursorWin& operator=(const CursorWin&) = delete;

  void* nativeHandle() override {
    return m_hcursor;
  }

private:
  HCURSOR m_hcursor;
};

// This class is used to avoid CreateDIBSection() as many times as
// possible (which is the slowest function if we are going to
// re-generate the cursor on each mouse movement).
class WinCursorCache {
public:
  WinCursorCache() { }
  ~WinCursorCache() { }

  bool recreate(const gfx::Size& size) {
    // Use cached bitmap
    if (m_hbmp && m_hmonobmp && m_size == size)
      return true;

    m_hbmp.reset();
    m_hmonobmp.reset();
    m_size = size;
    m_bits = nullptr;

    BITMAPV5HEADER bi;
    ZeroMemory(&bi, sizeof(BITMAPV5HEADER));
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = size.w;
    bi.bV5Height = size.h;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00ff0000;
    bi.bV5GreenMask = 0x0000ff00;
    bi.bV5BlueMask = 0x000000ff;
    bi.bV5AlphaMask = 0xff000000;

    HDC hdc = GetDC(nullptr);
    m_hbmp.reset(
      CreateDIBSection(
        hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS,
        (void**)&m_bits, NULL, (DWORD)0));
    ReleaseDC(nullptr, hdc);
    if (!m_hbmp) {
      m_bits = nullptr;
      return false;
    }

    return true;
  }

  HBITMAP hbmp() {
    ASSERT(m_hbmp);
    return m_hbmp.get();
  }

  // Create an empty mask bitmap.
  HBITMAP hmonobmp() {
    if (!m_hmonobmp) {
      // We must fill the mask bitmap with ones to avoid issues when a cursor is fully
      // transparent. Before this change we were returning a "no cursor" from makeCursor
      // when the cursor was fully transparent to avoid showing a black bitmap as the
      // cursor. But then the following issue was found:
      // https://github.com/aseprite/aseprite/issues/3989 (which is about the mouse
      // cursor leaving a trail of cursors after it was switched to a fully transparent
      // cursor bitmap.
      // By filling the mask with ones we fix both, the issue and the black cursor.
      int maskSize = (((m_size.w + 15) >> 4) << 1) * m_size.h;
      auto maskBits = std::make_unique<BYTE[]>(maskSize);
      std::memset(maskBits.get(), 0xFF, maskSize);
      m_hmonobmp.reset(CreateBitmap(m_size.w, m_size.h, 1, 1, maskBits.get()));
    }
    return m_hmonobmp.get();
  }

  uint32_t* bits() const {
    ASSERT(m_bits);
    return m_bits;
  }

private:
  HBitmapPtr m_hbmp;
  HBitmapPtr m_hmonobmp;
  uint32_t* m_bits = nullptr;
  gfx::Size m_size;
};

static WinCursorCache g_cursor_cache;

SystemWin::SystemWin()
  : m_screenMousePos(kUnknownPos)
{
}

SystemWin::~SystemWin()
{
  if (m_appMode == AppMode::GUI) {
    OleUninitialize();
  }
  destroyInstance();
}

void SystemWin::setAppMode(AppMode appMode)
{
  m_appMode = appMode;
  if (m_appMode == AppMode::GUI) {
    auto result = OleInitialize(nullptr);
    if (result != S_OK && result != S_FALSE) {
      LOG(LogLevel::ERROR, "WIN: Could not initialize OLE (%d)", result);
      return;
    }
  }
}

void SystemWin::setAppName(const std::string& appName)
{
  m_appName = appName;
}

void SystemWin::setTabletAPI(TabletAPI api)
{
  m_tabletAPI = api;

  // If the user selects the wintab API again, we remove any possible
  // file indicating a crash in the past.
  if (m_tabletAPI == TabletAPI::Wintab ||
      m_tabletAPI == TabletAPI::WintabPackets) {
    m_wintabApi.resetCrashFileIfPresent();
  }
}

bool SystemWin::isKeyPressed(KeyScancode scancode)
{
  return win_is_key_pressed(scancode);
}

int SystemWin::getUnicodeFromScancode(KeyScancode scancode)
{
  return win_get_unicode_from_scancode(scancode);
}

CursorRef SystemWin::makeCursor(const os::Surface* surface,
                                const gfx::Point& focus,
                                const int scale)
{
  ASSERT(surface);
  if (!surface)
    return nullptr;

  SurfaceFormatData format;
  surface->getFormat(&format);

  // Only for 32bpp surfaces
  if (format.bitsPerPixel != 32)
    return nullptr;

  gfx::Size sz(scale*surface->width(),
               scale*surface->height());

  if (!g_cursor_cache.recreate(sz))
    return nullptr;

  uint32_t* bits = g_cursor_cache.bits();
  for (int y=0; y<sz.h; ++y) {
    const uint32_t* ptr = (const uint32_t*)surface->getData(0, (sz.h-1-y)/scale);
    for (int x=0, u=0; x<sz.w; ++x, ++bits) {
      uint32_t c = *ptr;
      uint32_t a = ((c & format.alphaMask) >> format.alphaShift);

      *bits = (a << 24) |
        (((c & format.redMask  ) >> format.redShift  ) << 16) |
        (((c & format.greenMask) >> format.greenShift) << 8) |
        (((c & format.blueMask ) >> format.blueShift ));
      if (++u == scale) {
        u = 0;
        ++ptr;
      }
    }
  }

  ICONINFO ii;
  ii.fIcon = FALSE;
  ii.xHotspot = scale*focus.x + scale/2;
  ii.yHotspot = scale*focus.y + scale/2;
  ii.hbmMask = g_cursor_cache.hmonobmp();
  ii.hbmColor = g_cursor_cache.hbmp();

  HCURSOR hcursor = CreateIconIndirect(&ii);
  if (hcursor)
    return make_ref<CursorWin>(hcursor);
  else
    return nullptr;
}

gfx::Point SystemWin::mousePosition() const
{
  // We cannot use GetCursorPos() directly because it doesn't work
  // when have a pen connected to a notebook.
  if (m_screenMousePos != kUnknownPos) {
    return m_screenMousePos;
  }
  else {
    POINT pt;
    GetCursorPos(&pt);
    return gfx::Point(pt.x, pt.y);
  }
}

void SystemWin::setMousePosition(const gfx::Point& screenPosition)
{
  SetCursorPos(screenPosition.x, screenPosition.y);
}

gfx::Color SystemWin::getColorFromScreen(const gfx::Point& screenPosition) const
{
  HDC dc = GetDC(nullptr);
  COLORREF c = GetPixel(dc, screenPosition.x, screenPosition.y);
  ReleaseDC(nullptr, dc);
  return gfx::rgba(GetRValue(c),
                   GetGValue(c),
                   GetBValue(c));
}

ScreenRef SystemWin::mainScreen()
{
  // This is one of three possible ways to get the primary monitor
  // https://devblogs.microsoft.com/oldnewthing/20141106-00/?p=43683
  HMONITOR monitor = MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY);
  if (monitor)
    return make_ref<ScreenWin>(monitor);
  else
    return nullptr;
}

static BOOL CALLBACK list_screen_enumproc(HMONITOR monitor,
                                          HDC hdc, LPRECT rc,
                                          LPARAM data)
{
  auto list = (ScreenList*)data;
  list->push_back(make_ref<ScreenWin>(monitor));
  return TRUE;
}

void SystemWin::listScreens(ScreenList& list)
{
  EnumDisplayMonitors(
    nullptr, nullptr,
    list_screen_enumproc,
    (LPARAM)&list);
}

void SystemWin::_clearInternalMousePosition()
{
  m_screenMousePos = kUnknownPos;
}

void SystemWin::_setInternalMousePosition(const gfx::Point& pos)
{
  m_screenMousePos = pos;
}

void SystemWin::_setInternalMousePosition(const Event& ev)
{
  ASSERT(ev.window());
  if (!ev.window()) {           // Invalid Event state
    m_screenMousePos = kUnknownPos;
    return;
  }
  m_screenMousePos = ev.window()->pointToScreen(ev.position());
}

} // namespace os

#pragma pop_macro("ERROR")
