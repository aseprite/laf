// LAF OS Library
// Copyright (c) 2024-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/skia/skia_system.h"

#include "os/skia/skia_surface.h"
#include "os/skia/skia_window.h"

#include "include/core/SkGraphics.h"

namespace os {

SystemRef System::makeSkia()
{
  return make_ref<SkiaSystem>();
}

SkiaSystem::SkiaSystem()
{
  SkGraphics::Init();
}

SkiaSystem::~SkiaSystem()
{
  destroyInstance();
}

Capabilities SkiaSystem::capabilities() const
{
  return Capabilities(int(Capabilities::MultipleWindows) | int(Capabilities::CanResizeWindow) |
                      int(Capabilities::WindowScale) | int(Capabilities::CustomMouseCursor) |
                      int(Capabilities::ColorSpaces)
#ifndef LAF_MACOS
                      | int(Capabilities::CanStartWindowResize)
#endif
#if SK_SUPPORT_GPU
                      | int(Capabilities::GpuAccelerationSwitch)
#endif
  );
}

void SkiaSystem::setTabletOptions(const TabletOptions& options)
{
  SkiaSystemBase::setTabletOptions(options);
#if LAF_WINDOWS
  if (SkiaWindow* window = dynamic_cast<SkiaWindow*>(defaultWindow())) {
    // TODO notify all windows
    window->onTabletOptionsChange();
  }
#endif
}

os::Window* SkiaSystem::defaultWindow()
{
  return m_defaultWindow;
}

WindowRef SkiaSystem::makeWindow(const WindowSpec& spec)
{
  auto window = make_ref<SkiaWindow>(spec);
  if (!m_defaultWindow)
    m_defaultWindow = window.get();
  if (window && m_windowCS)
    window->setColorSpace(m_windowCS);
  return window;
}

SurfaceRef SkiaSystem::makeSurface(int width, int height, const os::ColorSpaceRef& colorSpace)
{
  auto sur = make_ref<SkiaSurface>();
  sur->create(width, height, colorSpace);
  return sur;
}

SurfaceRef SkiaSystem::makeRgbaSurface(int width, int height, const os::ColorSpaceRef& colorSpace)
{
  auto sur = make_ref<SkiaSurface>();
  sur->createRgba(width, height, colorSpace);
  return sur;
}

void SkiaSystem::setTextInput(bool state, const gfx::Point& screenCaretPos)
{
  if (m_defaultWindow)
    m_defaultWindow->setTextInput(state, screenCaretPos);
}

void SkiaSystem::setWindowsColorSpace(const os::ColorSpaceRef& cs)
{
  m_windowCS = cs;

  if (m_defaultWindow)
    m_defaultWindow->setColorSpace(m_windowCS);

  // TODO change the color space of all windows
}

} // namespace os
