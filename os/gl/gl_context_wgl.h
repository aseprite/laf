// LAF OS Library
// Copyright (C) 2022-present  Igara Studio S.A.
// Copyright (C) 2015-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_GL_CONTEXT_WGL_INCLUDED
#define OS_GL_CONTEXT_WGL_INCLUDED
#pragma once

#include "os/gpu_context.h"

#include <windows.h>

#include <gl/gl.h>

namespace os {

class GLContextWGL : public GpuContext {
public:
  GLContextWGL() : m_glrc(nullptr) {}

  ~GLContextWGL() { destroyContext(); }

  bool isValid() override { return m_glrc != nullptr; }

  bool makeContext(Window* window, GpuContext* shared) override
  {
    HWND hwnd = (HWND)window->nativeHandle();
    HDC hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,                     // version number
      PFD_DRAW_TO_WINDOW |   // support window
        PFD_SUPPORT_OPENGL | // support OpenGL
        PFD_DOUBLEBUFFER,    // double buffered
      PFD_TYPE_RGBA,         // RGBA type
      24,                    // 24-bit color depth
      0,
      0,
      0,
      0,
      0,
      0, // color bits ignored
      8, // 8-bit alpha buffer
      0, // shift bit ignored
      0, // no accumulation buffer
      0,
      0,
      0,
      0,              // accum bits ignored
      0,              // no z-buffer
      0,              // no stencil buffer
      0,              // no auxiliary buffer
      PFD_MAIN_PLANE, // main layer
      0,              // reserved
      0,
      0,
      0 // layer masks ignored
    };
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    m_glrc = wglCreateContext(hdc);
    if (!m_glrc) {
      ReleaseDC(hwnd, hdc);
      return false;
    }

    if (shared) {
      auto sharedGlrc = (HGLRC)shared->nativeHandle();
      wglShareLists(sharedGlrc, m_glrc);
    }

    wglMakeCurrent(hdc, m_glrc);
    ReleaseDC(hwnd, hdc);
    return true;
  }

  void destroyContext() override
  {
    if (m_glrc) {
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(m_glrc);
      m_glrc = nullptr;
    }
  }

  void makeCurrent(Window* window) override
  {
    HWND hwnd = (HWND)window->nativeHandle();
    HDC hdc = GetDC(hwnd);
    wglMakeCurrent(hdc, m_glrc);

    const gfx::Size clientSize = window->clientSize();
    glViewport(0, 0, clientSize.w, clientSize.h);

    ReleaseDC(hwnd, hdc);
  }

  void swapBuffers(Window* window) override
  {
    HWND hwnd = (HWND)window->nativeHandle();
    HDC hdc = GetDC(hwnd);
    SwapBuffers(hdc);
    ReleaseDC(hwnd, hdc);
  }

private:
  HGLRC m_glrc;
};

} // namespace os

#endif
