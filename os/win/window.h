// LAF OS Library
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_WINDOW_H_INCLUDED
#define OS_WIN_WINDOW_H_INCLUDED
#pragma once

#include "base/time.h"
#include "gfx/size.h"
#include "os/color_space.h"
#include "os/event.h"
#include "os/native_cursor.h"
#include "os/pointer_type.h"
#include "os/screen.h"
#include "os/tablet_options.h"
#include "os/win/dnd.h"
#include "os/win/wintab.h"

#include <interactioncontext.h>
#include <string>
#include <windows.h>

#define OS_USE_POINTER_API_FOR_MOUSE 0

namespace os {
class Surface;
class SystemWin;
class WindowSpec;

class WindowWin : public Window {
public:
  WindowWin(const WindowSpec& spec);
  ~WindowWin();

  os::ScreenRef screen() const override;
  os::ColorSpaceRef colorSpace() const override;
  int scale() const override { return m_scale; }
  int baseScale() const override { return m_baseScale; }
  void setScale(int scale) override;
  bool isVisible() const override;
  void setVisible(bool visible) override;
  void activate() override;
  void maximize() override;
  void minimize() override;
  bool isMaximized() const override;
  bool isMinimized() const override;
  bool isTransparent() const override;
  bool isFullscreen() const override;
  void setFullscreen(bool state) override;
  gfx::Size clientSize() const;
  gfx::Rect frame() const override;
  void setFrame(const gfx::Rect& bounds) override;
  gfx::Rect contentRect() const override;
  gfx::Rect restoredFrame() const override;
  std::string title() const override;
  void setTitle(const std::string& title) override;
  void captureMouse() override;
  void releaseMouse() override;
  void setMousePosition(const gfx::Point& position) override;
  bool setCursor(NativeCursor cursor) override;
  bool setCursor(const CursorRef& cursor) override;
  void performWindowAction(const WindowAction action, const Event* event) override;
  void invalidateRegion(const gfx::Region& rgn) override;
  std::string getLayout() override;

  void setLayout(const std::string& layout) override;
  void setTextInput(bool state, const gfx::Point& screenCaretPos = {});
  void setInterpretOneFingerGestureAsMouseMovement(bool state) override;
  void onTabletOptionsChange();

  NativeHandle nativeHandle() const override { return m_hwnd; }

protected:
  void onSetDragTarget() override;

private:
  bool setCursor(HCURSOR hcursor, const CursorRef& cursor);
  LRESULT wndProc(UINT msg, WPARAM wparam, LPARAM lparam);
  void mouseEvent(LPARAM lparam, Event& ev);
  bool pointerEvent(WPARAM wparam, Event& ev, POINTER_INFO& pi);
  void handleMouseMove(Event& ev);
  void handleMouseLeave();
  void handlePointerButtonChange(Event& ev, POINTER_INFO& pi);
  void handleInteractionContextOutput(const INTERACTION_CONTEXT_OUTPUT* output);

  void waitTimerToConvertFingerAsMouseMovement();
  void convertFingerAsMouseMovement();
  void delegateFingerToInteractionContext();
  void sendDelayedTouchEvents();
  void clearDelayedTouchEvents();
  void killTouchTimer();
  void checkColorSpaceChange();
  void checkDarkModeChange();

  void openWintabCtx();
  void closeWintabCtx();

  // Informs the taskbar that we are going (or exiting) the
  // fullscreen mode so it doesn't popup if it's hidden.
  void notifyFullScreenStateToShell();

  bool useScrollBarsHack() const
  {
    // TODO check if we can "return !m_usePointerApi" here in the
    //      future anyway it's not high-priority as we fixed the
    //      issue with the scrollbar grip
    return true;
  }

  TabletAPI tabletAPI() const;

  virtual void onResize(const gfx::Size& sz) {}
  virtual void onStartResizing() {}
  virtual void onEndResizing() {}
  virtual void onPaint(HDC hdc) {}
  virtual void onChangeColorSpace() {}

  static void registerClass();
  static HWND createHwnd(WindowWin* self, const WindowSpec& spec);
  static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  static void CALLBACK staticInteractionContextCallback(void* clientData,
                                                        const INTERACTION_CONTEXT_OUTPUT* output);

  // Returns a raw pointer to the current System instance, or nullptr
  // if it doesn't exist. It doesn't return a Ref because this pointer
  // can be requested from a ~WindowWin(), which is when the ~System
  // is already being called/destroyed.
  static SystemWin* system();

  mutable HWND m_hwnd = nullptr;
  HCURSOR m_hcursor = nullptr;
  CursorRef m_cursor;
  gfx::Size m_clientSize;

  // Used to store the current window position before toggling on
  // full-screen mode.
#if 0
    // TODO Restoring WINDOWPLACEMENT doesn't work well when the
    //      window is maximixed.
    WINDOWPLACEMENT m_restoredPlacement;
#else
  gfx::Rect m_restoredFrame;
#endif

  std::unique_ptr<DragTargetAdapter> m_dragTargetAdapter = nullptr;

  int m_scale;
  int m_baseScale;
  bool m_isCreated;
  // Since Windows Vista, it looks like Microsoft decided to change
  // the meaning of the window position to the shadow position (when
  // the DWM is enabled). So this flag is true in case we have to
  // adjust the real position we want to put the window.
  bool m_adjustShadow;
  bool m_textInput;
  bool m_hasMouse;
  bool m_captureMouse;

  // To change the color profile
  mutable std::string m_lastICCProfile;
  mutable os::ColorSpaceRef m_lastColorProfile;

  // Windows 8 pointer API
  bool m_usePointerApi;
  HINTERACTIONCONTEXT m_ictx;

  // This might be the most ugliest hack I've done to fix a Windows
  // bug. Here's the thing:
  // 1) When we use the pen on a Surface device, it send us
  //    WM_POINTERUPDATE messages, but after a WM_POINTERUPDATE
  //    we'll receive a WM_MOUSEMOVE with the position of the mouse
  //    (which is different to the position of the pen). This
  //    happens constantly if we press a modifier key like Alt,
  //    Ctrl, or Shift, and randomly if we don't use a modifier.
  // 2) First I tried to fix this issue disabling the WM_MOUSEMOVE
  //    processing when a pointer device is inside the window (with
  //    WM_POINTERENTER/LEAVE events), but that generated some buggy
  //    behavior (sometimes we stopped processing WM_MOUSEMOVE
  //    messages completely, it looks like a WM_POINTERLEAVE could
  //    be lost in time).
  // 3) Then I tried to fix this using the Windows pointer API for
  //    mouse messages (EnableMouseInPointer(TRUE)) but that API
  //    doesn't work well with Wacom drivers. And I guess it might
  //    depend on the specific driver version.
  // 4) Now I've reverted the pointer API change (we keep using the
  //    WM_MOUSE* messages) and fixed the original issue with this
  //    "m_ignoreRandomMouseEvents" variable. This is a counter that
  //    indicates the number of WM_MOUSEMOVE messages that we must
  //    ignore after a WM_POINTERUPDATE.
  // 5) Each time we receive a WM_POINTERUPDATE this counter is set
  //    to 2. A value that I calculated practically just testing the
  //    pen in a Surface Pro using the Alt modifier and seeing how
  //    many random WM_MOUSEMOVE messages were received.
  // 6) When a WM_MOUSEMOVE is received this counter is decreased,
  //    so when it backs to 0 we start processing WM_MOUSEMOVE
  //    messages again.
  int m_ignoreRandomMouseEvents;

  // Variables used to convert one finger in mouse-like movement,
  // and two/more fingers in scroll movement/pan/zoom. The idea
  // is as follows:
  // 1) When a PT_TOUCH is received, we count this event in
  //    m_fingers and setup a timer (m_fingerTimerID) to wait for
  //    another touch event.
  // 2) If another touch event is received, we process the messages
  //    with the interaction context (m_ictx) to handle special
  //    gestures (pan, magnify, etc.).
  // 3) If the timeout is reached, and we've received only one
  //    finger on the windows, we can send all awaiting events
  //    (m_fingerEvents) like mouse movement messages/button
  //    presses/releases.
  struct Touch {
    int fingers; // Number of fingers in the window
    // True when the timeout wasn't reached yet and the finger can be
    // converted to mouse events yet.
    bool canBeMouse;
    // True if we're already processing finger/touch events as mouse
    // movement events.
    bool asMouse;
    // Timeout (WM_TIMER) when the finger is converted to mouse events.
    UINT_PTR timerID;
    // Queued events to be sent when the finger is converted to mouse
    // events (these events are discarded if another finger is
    // introduced in the gesture e.g. to pan)
    std::vector<Event> delayedEvents;
    Touch();
  }* m_touch;

#if OS_USE_POINTER_API_FOR_MOUSE
  // Emulate double-click with pointer API. I guess that this should
  // be done by the Interaction Context API but it looks like
  // messages with pointerType != PT_TOUCH or PT_PEN are just
  // ignored by the ProcessPointerFramesInteractionContext()
  // function even when we call AddPointerInteractionContext() with
  // the given PT_MOUSE pointer.
  bool m_emulateDoubleClick;
  base::tick_t m_doubleClickMsecs;
  base::tick_t m_lastPointerDownTime;
  Event::MouseButton m_lastPointerDownButton;
  int m_pointerDownCount;
#endif

  // Wintab API data. The pointer type & pressure, and wintab
  // packets queue are shared between all windows, because it's
  // information from the device that doesn't depend on the active
  // window.
  bool m_allowWintab;
  HCTX m_hpenctx;
  static PointerType m_pointerType;
  static float m_pressure;
  static std::vector<PACKET> m_packets;
  static Event m_lastWintabEvent;

  // Flags
  bool m_fullscreen;
  bool m_titled;
  bool m_borderless;
  bool m_fixingPos;
  bool m_activate;

  bool m_firstVisible = false;
  bool m_startMaximized = false;
};

} // namespace os

#endif
