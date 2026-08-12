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
    XVisualInfo* vi = nullptr;

#if LAF_DEBUG_GPU
    auto glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB(
      (const GLubyte*)"glXCreateContextAttribsARB");

    if (glXCreateContextAttribsARB) {
      static const int attr[] = { GLX_CONTEXT_MAJOR_VERSION_ARB,
                                  4,
                                  GLX_CONTEXT_MINOR_VERSION_ARB,
                                  2,
                                  GLX_CONTEXT_FLAGS_ARB,
                                  GLX_CONTEXT_DEBUG_BIT_ARB |
                                    GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                                  GLX_CONTEXT_PROFILE_MASK_ARB,
                                  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                                  0 };
      static const int fbConfigAttr[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DOUBLEBUFFER, True, GLX_STENCIL_SIZE, 8, 0
      };

      int n = 0;
      GLXFBConfig* fbConfig =
        glXChooseFBConfig(m_display, DefaultScreen(m_display), fbConfigAttr, &n);
      if (n > 0) {
        vi = glXGetVisualFromFBConfig(m_display, *fbConfig);
        if (!vi)
          return false;
      }

      if (fbConfig) {
        m_glCtx = glXCreateContextAttribsARB(m_display, *fbConfig, sharedGpuContext, GL_TRUE, attr);
        XFree(fbConfig);
      }
    }
#endif

    if (!m_glCtx) {
      GLint attr[] = { GLX_RGBA, GLX_DOUBLEBUFFER, 0 };
      vi = glXChooseVisual(m_display, 0, attr);
      if (!vi)
        return false;

      m_glCtx = glXCreateContext(m_display, vi, sharedGpuContext, GL_TRUE);
      XFree(vi);
    }

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
