// LAF OS Library
// Copyright (C) 2018-present  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_WINDOW_H_INCLUDED
#define OS_OSX_WINDOW_H_INCLUDED
#pragma once

#ifdef __OBJC__
  #include <Cocoa/Cocoa.h>
#endif

#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "os/keys.h"
#include "os/native_cursor.h"
#include "os/osx/color_space.h"
#include "os/osx/screen.h"
#include "os/osx/view.h"
#include "os/system.h"
#include "os/window.h"
#include "os/window_spec.h"

namespace os {
class Event;
class Surface;
class WindowOSX;
} // namespace os

#ifdef __OBJC__

@class WindowOSXDelegate;

@interface WindowOSXObjc : NSWindow {
@private
  os::WindowOSX* __weak m_impl;
  WindowOSXDelegate* __strong m_delegate;
  ViewOSX* __strong m_view;
  int m_scale;
  bool m_floating;
}
- (WindowOSXObjc*)initWithImpl:(os::WindowOSX*)impl spec:(const os::WindowSpec*)spec;
- (os::WindowOSX*)impl;
- (void)removeImpl;
- (int)scale;
- (void)setScale:(int)scale;
- (gfx::Size)clientSize;
- (void)setMousePosition:(const gfx::Point&)position;
- (BOOL)setNativeCursor:(os::NativeCursor)cursor;
- (BOOL)canBecomeKeyWindow;
- (BOOL)isFloating;
@end

using WindowOSXObjc_id = WindowOSXObjc*;

#else

  #include <objc/objc-runtime.h>
using WindowOSXObjc_id = id;

#endif

namespace os {

class WindowOSX : public os::Window {
public:
  void createWindow(const os::WindowSpec& spec);
  void destroyWindow();

  gfx::Size clientSize() const override;
  gfx::Rect frame() const override;
  void setFrame(const gfx::Rect& bounds) override;
  gfx::Rect contentRect() const override;
  gfx::Rect restoredFrame() const override;

  void activate() override;
  void maximize() override;
  void minimize() override;
  bool isMaximized() const override;
  bool isMinimized() const override;
  bool isTransparent() const override;
  bool isFullscreen() const override;
  void setFullscreen(bool state) override;

  std::string title() const override;
  void setTitle(const std::string& title) override;

  void captureMouse() override;
  void releaseMouse() override;
  void setMousePosition(const gfx::Point& position) override;

  void performWindowAction(const WindowAction action, const Event* event) override;

  os::ScreenRef screen() const override;
  os::ColorSpaceRef colorSpace() const override;

  int scale() const override;
  void setScale(int scale) override;
  bool isVisible() const override;
  void setVisible(bool visible) override;

  bool setCursor(NativeCursor cursor) override;
  bool setCursor(const CursorRef& cursor) override;

  void* nativeHandle() const override;

  virtual void onClose() = 0;
  virtual void onResize(const gfx::Size& size) = 0;
  virtual void onDrawRect(const gfx::Rect& rect) = 0;
  virtual void onWindowChanged() = 0;
  virtual void onStartResizing() = 0;
  virtual void onResizing(gfx::Size& size) = 0;
  virtual void onEndResizing() = 0;
  virtual void onBeforeMaximizeFrame();

  // This generally happens when the window is moved to another
  // monitor with different scale (e.g. Retina vs non-Retina display)
  // or when the color space changes.
  virtual void onChangeBackingProperties() = 0;

protected:
  WindowOSXObjc_id __strong m_nsWindow = nullptr;
  gfx::Rect m_restoredFrame; // Cached frame() just before it's maximized/zoomed.
};

} // namespace os

#endif
