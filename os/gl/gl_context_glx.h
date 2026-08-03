// LAF OS Library
// Copyright (C) 2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_GL_CONTEXT_GLX_INCLUDED
#define OS_GL_CONTEXT_GLX_INCLUDED
#pragma once

#include "os/gl/gl_context.h"

#include <GL/glx.h>
#include <X11/Xlib.h>
#undef None

namespace os {

class GLContextGLX : public GLContext {
public:
  GLContextGLX(::Display* display, ::Window window) : m_display(display), m_window(window) {}

  ~GLContextGLX() { destroyGLContext(); }

  bool isValid() override { return m_glCtx != nullptr; }

  bool createGLContext() override
  {
    GLint attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, 0 };
    XVisualInfo* vi = glXChooseVisual(m_display, 0, attr);
    if (!vi)
      return false;

    m_glCtx = glXCreateContext(m_display, vi, nullptr, GL_TRUE);
    if (!m_glCtx)
      return false;

    glClearStencil(0);
    glClearColor(0, 0, 0, 0);
    glStencilMask(0xffffffff);
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    ::Window root;
    int x, y, w, h;
    unsigned int border_width, depth;
    XGetGeometry(m_display,
                 m_window,
                 &root,
                 &x,
                 &y,
                 (unsigned int*)&w,
                 (unsigned int*)&h,
                 &border_width,
                 &depth);
    glViewport(0, 0, w, h);
    glDrawBuffer(GL_BACK);

    return true;
  }

  void destroyGLContext() override
  {
    if (m_glCtx) {
      glXDestroyContext(m_display, m_glCtx);
      m_glCtx = nullptr;
    }
  }

  void makeCurrent() override { glXMakeCurrent(m_display, m_window, m_glCtx); }

  void swapBuffers() override { glXSwapBuffers(m_display, m_window); }

private:
  ::Display* m_display = nullptr;
  ::Window m_window = 0;
  GLXContext m_glCtx = nullptr;
};

} // namespace os

#endif
