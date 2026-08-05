// LAF OS Library
// Copyright (C) 2022-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_GL_CONTEXT_GLX_INCLUDED
#define OS_GL_CONTEXT_GLX_INCLUDED
#pragma once

#include "os/gpu_context.h"
#include "os/window.h"

#include <GL/glx.h>
#include <X11/Xlib.h>
#undef None

namespace os {

class GLContextGLX : public GpuContext {
public:
  GLContextGLX(::Display* display) : m_display(display) {}

  ~GLContextGLX() { destroyContext(); }

  bool isValid() override { return m_glCtx != nullptr; }

  bool makeContext(Window* window, GpuContext* shared) override
  {
    auto sharedGpuContext = (shared ? (GLXContext)shared->nativeHandle() : nullptr);

    GLint attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, 0 };
    XVisualInfo* vi = glXChooseVisual(m_display, 0, attr);
    if (!vi)
      return false;

    m_glCtx = glXCreateContext(m_display, vi, sharedGpuContext, GL_TRUE);
    XFree(vi);
    if (!m_glCtx)
      return false;

    makeCurrent(window);

    glClearStencil(0);
    glClearColor(0, 0, 0, 0);
    glStencilMask(0xffffffff);
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDrawBuffer(GL_BACK);

    return true;
  }

  void destroyContext() override
  {
    if (m_glCtx) {
      glXDestroyContext(m_display, m_glCtx);
      m_glCtx = nullptr;
    }
  }

  void makeCurrent(Window* window) override
  {
    glXMakeCurrent(m_display, (::Window)window->nativeHandle(), m_glCtx);

    const gfx::Size clientSize = window->clientSize();
    glViewport(0, 0, clientSize.w, clientSize.h);
  }

  void swapBuffers(Window* window) override
  {
    glXSwapBuffers(m_display, (::Window)window->nativeHandle());
  }

private:
  ::Display* m_display = nullptr;
  GLXContext m_glCtx = nullptr;
};

} // namespace os

#endif
