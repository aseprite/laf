// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_IOS_WINDOW_H_INCLUDED
#define OS_IOS_WINDOW_H_INCLUDED
#pragma once

#ifdef __OBJC__
  #import <UIKit/UIKit.h>
#endif

#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "os/keys.h"
#include "os/native_cursor.h"
#include "os/system.h"
#include "os/window.h"
#include "os/window_spec.h"

namespace os {
class Event;
class Surface;
class WindowIOS;
} // namespace os

#ifdef __OBJC__

using UIWindowIOS_id = UIWindow*;
@class AsepriteView;

#else

  #include <objc/runtime.h>
  #include <objc/message.h>
using UIWindowIOS_id = id;
using AsepriteView = id;

#endif

namespace os {

class WindowIOS : public os::Window {
public:
  void createWindow(const os::WindowSpec& spec);
  void destroyWindow();

  gfx::Size clientSize() const;
  gfx::Rect frame() const override;
  void setFrame(const gfx::Rect& bounds) override;
  gfx::Rect contentRect() const override;
  gfx::Rect restoredFrame() const override;

  void activate() override;
  void maximize() override {}
  void minimize() override {}
  bool isMaximized() const override { return false; }
  bool isMinimized() const override { return false; }
  bool isTransparent() const override { return false; }
  bool isFullscreen() const override { return true; }
  void setFullscreen(bool state) override {}

  std::string title() const override;
  void setTitle(const std::string& title) override;

  void captureMouse() override {}
  void releaseMouse() override {}
  void setMousePosition(const gfx::Point& position) override {}

  void performWindowAction(const WindowAction action, const Event* event) override {}

  os::ScreenRef screen() const override { return nullptr; }
  os::ColorSpaceRef colorSpace() const override;

  int scale() const override;
  void setScale(int scale) override;
  bool isVisible() const override;
  void setVisible(bool visible) override;

  bool setCursor(NativeCursor cursor) override { return true; }
  bool setCursor(const CursorRef& cursor) override { return true; }

  void* nativeHandle() const override;

  AsepriteView* asepriteView() const;

  virtual void onClose() = 0;
  virtual void onResize(const gfx::Size& size) = 0;
  virtual void onDrawRect(const gfx::Rect& rect) = 0;

protected:
  UIWindowIOS_id __strong m_uiWindow = nullptr;
  int m_scale = 1;
};

} // namespace os

#endif
