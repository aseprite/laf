// LAF OS Library
// Copyright (c) 2018-2024  Igara Studio S.A.
// Copyright (c) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WINDOW_H_INCLUDED
#define OS_WINDOW_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "os/color_space.h"
#include "os/cursor.h"
#include "os/dnd.h"
#include "os/native_cursor.h"
#include "os/ref.h"
#include "os/screen.h"
#include "os/surface_list.h"

#include <functional>
#include <string>

#pragma push_macro("None")
#undef None // Undefine the X11 None macro

#if LAF_SKIA && SK_SUPPORT_GPU
class GrDirectContext;
#endif

namespace os {

class Event;
class Surface;
class Window;
using WindowRef = Ref<Window>;

enum class WindowAction {
  Cancel,
  Move,
  ResizeFromTopLeft,
  ResizeFromTop,
  ResizeFromTopRight,
  ResizeFromLeft,
  ResizeFromRight,
  ResizeFromBottomLeft,
  ResizeFromBottom,
  ResizeFromBottomRight,
};

// Possible areas for hit testing. See os::Window::handleHitTest.
enum class Hit {
  None,
  Content,
  TitleBar,
  TopLeft,
  Top,
  TopRight,
  Left,
  Right,
  BottomLeft,
  Bottom,
  BottomRight,
  MinimizeButton,
  MaximizeButton,
  CloseButton,
};

// A window to show graphics.
class Window : public RefCount {
public:
  typedef void* NativeHandle;

  virtual ~Window() {}

  // Real rectangle of this window (including title bar, etc.) in
  // the screen. (The scale is not involved.)
  virtual gfx::Rect frame() const = 0;
  virtual void setFrame(const gfx::Rect& bounds) = 0;

  // Rectangle of the content, the origin and the size are specified
  // in real screen pixels.  (The scale is not involved.)
  virtual gfx::Rect contentRect() const = 0;

  // Returns the frame() when the window wasn't maximized or in
  // full-screen mode.
  virtual gfx::Rect restoredFrame() const = 0;

  // Returns the real and current window's size (without scale applied).
  virtual int width() const = 0;
  virtual int height() const = 0;
  gfx::Rect bounds() const;

  // Returns the current window scale. Each pixel in the internal
  // window surface, is represented by SCALExSCALE pixels on the
  // screen.
  virtual int scale() const = 0;

  // Changes the scale.
  // The available surface size will be (Window::width() / scale,
  //                                     Window::height() / scale)
  //
  // The window size will be a multiple of the scale.
  virtual void setScale(int scale) = 0;

  // Returns the current window scale for default DPI value.
  // It is constant across different displays.
  virtual int baseScale() const { return scale(); }

  // Returns true if the window is visible in the screen.
  virtual bool isVisible() const = 0;

  // Shows or hides the window (doesn't destroy it).
  virtual void setVisible(bool visible) = 0;

  // Returns the main surface to draw into this window.
  virtual Surface* surface() = 0;

  // Invalidates part of the window to be redraw in the future by
  // the OS painting messages. The region must be in non-scaled
  // coordinates.
  virtual void invalidateRegion(const gfx::Region& rgn) = 0;
  void invalidate();

  // GPU-related functions
  virtual bool gpuAcceleration() const = 0;
  virtual void setGpuAcceleration(bool state) {}
  virtual void swapBuffers() = 0;

  // Focus the window to receive the keyboard input by default.
  virtual void activate() = 0;
  virtual void maximize() = 0;
  virtual void minimize() = 0;
  virtual bool isMaximized() const = 0;
  virtual bool isMinimized() const = 0;

  // Returns true if this native window is
  // transparent. E.g. WS_EX_LAYERED on Windows,
  // or [NSWindow isOpaque:NO] on macOS.
  virtual bool isTransparent() const = 0;

  virtual bool isFullscreen() const = 0;
  virtual void setFullscreen(bool state) = 0;

  virtual std::string title() const = 0;
  virtual void setTitle(const std::string& title) = 0;

  virtual void setIcons(const SurfaceList& icons) {};

  virtual NativeCursor nativeCursor() = 0;
  virtual bool setCursor(NativeCursor cursor) = 0;
  virtual bool setCursor(const CursorRef& cursor) = 0;

  // Sets the mouse position to the given point in surface coordinates.
  virtual void setMousePosition(const gfx::Point& position) = 0;

  virtual void captureMouse() = 0;
  virtual void releaseMouse() = 0;

  // Convert points between window surface bounds (scaled) <-> screen absolute position
  gfx::Point pointToScreen(const gfx::Point& clientPosition) const;
  gfx::Point pointFromScreen(const gfx::Point& screenPosition) const;

  // Queue event for this window (the "ev" window will be set to
  // this window if it's not set).
  void queueEvent(os::Event& ev);

  // Performs the user action to move or resize the window. It's
  // useful in case that you want to design your own regions to
  // resize or move/drag the window.
  virtual void performWindowAction(WindowAction action, const Event* event = nullptr) = 0;

  // Set/get the specific information to restore the exact same
  // window position (e.g. in the same monitor).
  virtual std::string getLayout() = 0;
  virtual void setLayout(const std::string& layout) = 0;

  // For Windows 8/10 only in tablet devices: Set to true if you
  // want to interpret one finger as the mouse movement and two
  // fingers as pan/scroll (true by default). If you want to pan
  // with one finger, call this function with false.
  virtual void setInterpretOneFingerGestureAsMouseMovement(bool state) {}

  // Returns the screen where this window belongs.
  virtual os::ScreenRef screen() const = 0;

  // Returns the color space of the window where the window is located.
  virtual os::ColorSpaceRef colorSpace() const = 0;

  // Changes the color space to use in this window. Can be nullptr
  // if you want to use the current monitor color space.
  virtual void setColorSpace(const os::ColorSpaceRef& colorSpace) = 0;

  // Returns the HWND on Windows, X11 Window, or bridged NSWindow pointer.
  virtual NativeHandle nativeHandle() const = 0;

  // For Windows platform, function used to handle WM_NCHITTEST for
  // borderless windows (mainly). It's required so some input
  // devices like stylus can be used to move and resize the window.
  std::function<Hit(os::Window*, const gfx::Point& pos)> handleHitTest = nullptr;

  void setDragTarget(DragTarget* delegate);
  bool hasDragTarget() { return m_dragTarget != nullptr; }

  template<typename T>
  T* userData()
  {
    return reinterpret_cast<T*>(m_userData);
  }

  template<typename T>
  void setUserData(T* data)
  {
    m_userData = reinterpret_cast<void*>(data);
  }

#if LAF_SKIA && SK_SUPPORT_GPU
  virtual GrDirectContext* sk_grCtx() const = 0;
#endif

  void notifyDragEnter(os::DragEvent& ev);
  void notifyDrag(os::DragEvent& ev);
  void notifyDragLeave(os::DragEvent& ev);
  void notifyDrop(os::DragEvent& ev);

protected:
  virtual void onQueueEvent(Event& ev);
  virtual void onDragEnter(os::DragEvent& ev);
  virtual void onDrag(os::DragEvent& ev);
  virtual void onDragLeave(os::DragEvent& ev);
  virtual void onDrop(os::DragEvent& ev);

  virtual void onSetDragTarget() {}

private:
  void* m_userData;
  DragTarget* m_dragTarget = nullptr;
};

} // namespace os

#pragma pop_macro("None")

#endif
