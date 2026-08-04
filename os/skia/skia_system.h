// LAF OS Library
// Copyright (C) 2018-present  Igara Studio S.A.
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

#include <algorithm>
#include <memory>

namespace os {

class SkiaWindow;

class SkiaSystem final : public SkiaSystemBase {
public:
  SkiaSystem();
  ~SkiaSystem();

  Capabilities capabilities() const override;

  void setTabletOptions(const TabletOptions& options) override;

  Window* defaultWindow() override;

  WindowRef makeWindow(const WindowSpec& spec) override;
  SurfaceRef makeSurface(int width, int height, const os::ColorSpaceRef& colorSpace) override;
  SurfaceRef makeRgbaSurface(int width, int height, const os::ColorSpaceRef& colorSpace) override;

  SurfaceRef loadSurface(const char* filename) override
  {
    return SkiaSurface::loadSurface(filename);
  }

  SurfaceRef loadRgbaSurface(const char* filename) override { return loadSurface(filename); }

  void setTextInput(bool state, const gfx::Point& screenCaretPos = {}) override;

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

  void setWindowsColorSpace(const os::ColorSpaceRef& cs) override;

  os::ColorSpaceRef windowsColorSpace() override { return m_windowCS; }

private:
  SkiaWindow* m_defaultWindow = nullptr;
  ColorSpaceRef m_windowCS;
};

} // namespace os

#endif
