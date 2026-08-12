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

  virtual SurfaceRef makeOnscreenRenderTarget(const gfx::Size& size, const os::ColorSpaceRef& cs)
  {
    return {};
  }

  virtual SurfaceRef makeOffscreenRenderTarget(const gfx::Size& size, const os::ColorSpaceRef& cs)
  {
    return {};
  }
};

} // namespace os

#endif
