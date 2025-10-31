// LAF OS Library
// Copyright (c) 2020-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_SCREEN_H_INCLUDED
#define OS_OSX_SCREEN_H_INCLUDED
#pragma once

#ifdef __OBJC__

  #include "os/osx/color_space.h"
  #include "os/screen.h"

  #include <Cocoa/Cocoa.h>

namespace os {

class ScreenOSX : public Screen {
public:
  ScreenOSX(NSScreen* screen) : m_screen(screen)
  {
    auto rc = screen.frame;
    auto wa = screen.visibleFrame;
    const int mainScreenHeight = [NSScreen mainScreen].frame.size.height;

    m_bounds.x = rc.origin.x;
    m_bounds.y = mainScreenHeight - rc.origin.y - rc.size.height;
    m_bounds.w = rc.size.width;
    m_bounds.h = rc.size.height;

    m_workarea.x = wa.origin.x;
    m_workarea.y = mainScreenHeight - wa.origin.y - wa.size.height;
    m_workarea.w = wa.size.width;
    m_workarea.h = wa.size.height;
  }
  bool isMainScreen() const override { return m_screen == [NSScreen mainScreen]; }
  gfx::Rect bounds() const override { return m_bounds; }
  gfx::Rect workarea() const override { return m_workarea; }
  os::ColorSpaceRef colorSpace() const override
  {
    return convert_nscolorspace_to_os_colorspace([m_screen colorSpace]);
  }
  void* nativeHandle() const override { return (__bridge void*)m_screen; }

private:
  NSScreen* m_screen;
  gfx::Rect m_bounds;
  gfx::Rect m_workarea;
};

} // namespace os

#endif // __OBJC__

#endif
