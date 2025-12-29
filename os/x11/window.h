// LAF OS Library
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_WINDOW_INCLUDED
#define OS_X11_WINDOW_INCLUDED
#pragma once

#include "base/time.h"
#include "gfx/border.h"
#include "gfx/color_space.h" // Include here avoid error with None
#include "gfx/fwd.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "os/color_space.h"
#include "os/event.h"
#include "os/native_cursor.h"
#include "os/screen.h"
#include "os/surface_list.h"
#include "os/window.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstring>
#include <string>

namespace os {

class Surface;
class WindowSpec;

class WindowX11 : public Window {
public:
  WindowX11(::Display* display, const WindowSpec& spec);
  ~WindowX11();

  os::ScreenRef screen() const override;
  os::ColorSpaceRef colorSpace() const override;

  int scale() const override { return m_scale; }
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

  void setTitle(const std::string& title) override;
  void setIcons(const SurfaceList& icons) override;

  gfx::Rect frame() const override;
  void setFrame(const gfx::Rect& bounds) override;
  gfx::Rect contentRect() const override;
  gfx::Rect restoredFrame() const override;
  std::string title() const override;

  gfx::Size clientSize() const;
  void captureMouse() override;
  void releaseMouse() override;
  void setMousePosition(const gfx::Point& position) override;
  void invalidateRegion(const gfx::Region& rgn) override;
  bool setCursor(NativeCursor cursor) override;
  bool setCursor(const CursorRef& cursor) override;

  void performWindowAction(WindowAction action, const Event* event) override;

  ::Display* x11display() const { return m_display; }
  ::Window x11window() const { return m_window; }
  ::GC gc() const { return m_gc; }

  NativeHandle nativeHandle() const override { return (NativeHandle)x11window(); }

  void setTextInput(bool state, const gfx::Point& screenCaretPos = {}) { g_textInput = state; }

  static bool textInput() { return g_textInput; }

  void processX11Event(XEvent& event);
  static WindowX11* getPointerFromHandle(::Window handle);

  // Only used for debugging purposes.
  static size_t countActiveWindows();

protected:
  virtual void onPaint(const gfx::Rect& rc) = 0;
  virtual void onResize(const gfx::Size& sz) = 0;

private:
  void setWMClass(const std::string& res_class);
  void setAllowedActions();
  bool setX11Cursor(::Cursor xcursor);
  bool requestX11FrameExtents();
  void getX11FrameExtents();

  bool isDoubleClickEvent(const Event& event) const;
  void handleXInputDoubleClickEvent(int button, Event& ev);

  static void addWindow(WindowX11* window);
  static void removeWindow(WindowX11* window);

  ::Display* m_display;
  ::Window m_window;
  ::GC m_gc;
  ::XIC m_xic;
  int m_scale;
  gfx::Point m_lastMousePos;
  gfx::Rect m_lastConfigure;
  gfx::Border m_frameExtents;
  bool m_initializingActions = true;
  bool m_fullscreen = false;
  bool m_borderless = false;
  bool m_closable = false;
  bool m_maximizable = false;
  bool m_minimizable = false;
  bool m_resizable = false;
  bool m_transparent = false;

  // Double-click info
  Event::MouseButton m_doubleClickButton;
  base::tick_t m_doubleClickTick;
  gfx::Point m_doubleClickStartPos;
  static const int kDoubleClickThreshold = 8;

  static bool g_textInput;
};

} // namespace os

#endif
