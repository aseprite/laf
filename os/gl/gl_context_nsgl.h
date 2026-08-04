// LAF OS Library
// Copyright (C) 2022-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_GL_CONTEXT_NSGL_INCLUDED
#define OS_GL_CONTEXT_NSGL_INCLUDED
#pragma once

#include "gfx/size.h"
#include "os/gpu_context.h"

namespace os {

class GLContextNSGL : public GpuContext {
public:
  GLContextNSGL();
  ~GLContextNSGL();

  void setView(id view);

  bool isValid() override;

  bool makeContext(Window* window, GpuContext* shared) override;
  void destroyContext() override;
  void* nativeHandle() override { return (__bridge void*)m_nsgl; }

  void makeCurrent(Window* window) override;
  void swapBuffers(Window* window) override;

  id nsglContext() { return m_nsgl; }

private:
  id m_nsgl = nullptr; // NSOpenGLContext
  id m_view = nullptr; // NSView
  gfx::Size m_lastClientSize;
};

} // namespace os

#endif
