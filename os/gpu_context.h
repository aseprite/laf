// LAF OS Library
// Copyright (C) 2022-present  Igara Studio S.A.
// Copyright (C) 2015-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_GPU_CONTEXT_H_INCLUDED
#define OS_GPU_CONTEXT_H_INCLUDED
#pragma once

#include "os/surface.h"

#include <memory>

namespace os {

class Window;

// "surface" is where we draw the content scaled (e.g. smaller than
// the final result). The "fbSurface" points to the framebuffer where
// we draw (e.g. GL_BACK) which is unscaled/real pixels. Before a
// SwapBuffers we copy the "surface" into "fbSurface", but only if
// scale != 1, if scale = 1 both surfaces match.
struct RenderTarget {
  os::SurfaceRef fbSurface; // Framebuffer surface (real pixels / unscaled)
  os::SurfaceRef surface;   // Scaled surface to paint
};

class GpuContext {
public:
  virtual ~GpuContext() {}
  virtual bool isValid() { return false; }

  virtual bool makeContext(Window* window, GpuContext* shared) { return false; }
  virtual void destroyContext() {}
  virtual void* nativeHandle() { return nullptr; }

  virtual void makeCurrent(Window* window) {}
  virtual void swapBuffers(Window* window) {}
  virtual void flush() {}
  virtual RenderTarget makeRenderTarget(const gfx::Size& size,
                                        const int scale,
                                        const os::ColorSpaceRef& cs)
  {
    return {};
  }
};

} // namespace os

#endif
