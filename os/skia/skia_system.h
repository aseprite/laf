// LAF OS Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_SYSTEM_INCLUDED
#define OS_SKIA_SKIA_SYSTEM_INCLUDED
#pragma once

#include "gfx/color_space.h"
#include "gfx/size.h"
#include "os/common/system.h"
#include "os/skia/skia_color_space.h"
#include "os/skia/skia_surface.h"
#include "os/skia/skia_window.h"
#include "os/surface_format.h"
#include "os/window_spec.h"

#if LAF_WINDOWS
  #include "os/win/color_space.h"
  #include "os/win/system.h"
  #define SkiaSystemBase SystemWin
#elif LAF_MACOS
  #include "os/osx/color_space.h"
  #include "os/osx/system.h"
  #define SkiaSystemBase SystemOSX
#elif LAF_LINUX
  #include "os/x11/system.h"
  #define SkiaSystemBase SystemX11
#endif

#include "include/core/SkGraphics.h"

#include <algorithm>
#include <memory>

namespace os {

class SkiaSystem final : public SkiaSystemBase {
public:
  SkiaSystem() : m_defaultWindow(nullptr) { SkGraphics::Init(); }

  ~SkiaSystem() { destroyInstance(); }

  Capabilities capabilities() const override
  {
    return Capabilities(int(Capabilities::MultipleWindows) | int(Capabilities::CanResizeWindow) |
                        int(Capabilities::WindowScale) | int(Capabilities::CustomMouseCursor) |
                        int(Capabilities::ColorSpaces)
#ifndef __APPLE__
                        | int(Capabilities::CanStartWindowResize)
#endif
#if SK_SUPPORT_GPU
                        | int(Capabilities::GpuAccelerationSwitch)
#endif
    );
  }

  void setTabletOptions(const TabletOptions& options) override
  {
    SkiaSystemBase::setTabletOptions(options);
#if LAF_WINDOWS
    if (SkiaWindow* window = dynamic_cast<SkiaWindow*>(defaultWindow())) {
      // TODO notify all windows
      window->onTabletOptionsChange();
    }
#endif
  }

  Window* defaultWindow() override { return m_defaultWindow; }

  WindowRef makeWindow(const WindowSpec& spec) override
  {
    auto window = make_ref<SkiaWindow>(spec);
    if (!m_defaultWindow)
      m_defaultWindow = window.get();
    if (window && m_windowCS)
      window->setColorSpace(m_windowCS);
    return window;
  }

  SurfaceRef makeSurface(int width, int height, const os::ColorSpaceRef& colorSpace) override
  {
    auto sur = make_ref<SkiaSurface>();
    sur->create(width, height, colorSpace);
    return sur;
  }

  SurfaceRef makeRgbaSurface(int width, int height, const os::ColorSpaceRef& colorSpace) override
  {
    auto sur = make_ref<SkiaSurface>();
    sur->createRgba(width, height, colorSpace);
    return sur;
  }

  SurfaceRef loadSurface(const char* filename) override
  {
    return SkiaSurface::loadSurface(filename);
  }

  SurfaceRef loadRgbaSurface(const char* filename) override { return loadSurface(filename); }

  void setTextInput(bool state, const gfx::Point& screenCaretPos = {}) override
  {
    if (m_defaultWindow)
      m_defaultWindow->setTextInput(state, screenCaretPos);
  }

  void listColorSpaces(std::vector<os::ColorSpaceRef>& list) override
  {
    list.push_back(makeColorSpace(gfx::ColorSpace::MakeNone()));
    list.push_back(makeColorSpace(gfx::ColorSpace::MakeSRGB()));

#if LAF_WINDOWS || LAF_MACOS
    list_display_colorspaces(list);
#endif
  }

  os::ColorSpaceRef makeColorSpace(const gfx::ColorSpaceRef& cs) override
  {
    return os::make_ref<SkiaColorSpace>(cs);
  }

  Ref<ColorSpaceConversion> convertBetweenColorSpace(const os::ColorSpaceRef& src,
                                                     const os::ColorSpaceRef& dst) override
  {
    return os::make_ref<SkiaColorSpaceConversion>(src, dst);
  }

  void setWindowsColorSpace(const os::ColorSpaceRef& cs) override
  {
    m_windowCS = cs;

    if (m_defaultWindow)
      m_defaultWindow->setColorSpace(m_windowCS);

    // TODO change the color space of all windows
  }

  os::ColorSpaceRef windowsColorSpace() override { return m_windowCS; }

private:
  SkiaWindow* m_defaultWindow;
  bool m_gpuAcceleration;
  ColorSpaceRef m_windowCS;
};

} // namespace os

#endif
