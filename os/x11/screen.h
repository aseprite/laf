// LAF OS Library
// Copyright (c) 2020-2021  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_SCREEN_H
#define OS_X11_SCREEN_H
#pragma once

#include "os/screen.h"
#include "os/x11/monitor.h"
#include "os/x11/x11.h"
#include "os/system.h"

#include <X11/extensions/Xrandr.h>

namespace os {

class ScreenX11 : public Screen {
public:
  ScreenX11(int monitorNum) : m_monitorNum(monitorNum) {
    MonitorsX11* monitors = X11::instance()->monitors();
    const XRRMonitorInfo& monitor = monitors->monitorInfo(monitorNum);

    m_bounds.x = monitor.x;
    m_bounds.y = monitor.y;
    m_bounds.w = monitor.width;
    m_bounds.h = monitor.height;
    // Xorg doesn't really provide a way to find workarea per monitor :/
    m_workarea.x = monitor.x;
    m_workarea.y = monitor.y;
    m_workarea.w = monitor.width;
    m_workarea.h = monitor.height;

    m_isPrimary = monitor.primary;
  }

  bool isMainScreen() const override { return m_isPrimary; }
  gfx::Rect bounds() const override { return m_bounds; }
  gfx::Rect workarea() const override { return m_workarea; }
  os::ColorSpaceRef colorSpace() const override {
    // TODO get screen color space
    return os::instance()->makeColorSpace(gfx::ColorSpace::MakeSRGB());
  }
  void* nativeHandle() const override {
    return reinterpret_cast<void*>(m_monitorNum);
  }
private:
  int m_monitorNum;
  bool m_isPrimary;
  gfx::Rect m_bounds;
  gfx::Rect m_workarea;
};

} // namespace os

#endif
