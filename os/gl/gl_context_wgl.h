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

#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_FLAGS_ARB                     0x2094

#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC(WINAPI*)(HDC, HGLRC, const int*);

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
    HGLRC sharedGlrc = (shared ? (HGLRC)shared->nativeHandle() : nullptr);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

#if LAF_DEBUG_GPU
    // TODO It doesn't work, it looks like we have to create a temporary HWND
    HWND desktopHdc = GetDC(desktopHdc);
    HGLRC dummyRc = wglCreateContext(desktopHdc);

    auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress(
      "wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB) {
      int attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB,
                        4,
                        WGL_CONTEXT_MINOR_VERSION_ARB,
                        2,
                        WGL_CONTEXT_FLAGS_ARB,
                        WGL_CONTEXT_DEBUG_BIT_ARB | WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                        WGL_CONTEXT_PROFILE_MASK_ARB,
                        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                        0 };
      m_glrc = wglCreateContextAttribsARB(hdc, sharedGlrc, attribs);
    }

    wglDeleteContext(dummyRc);
    ReleaseDC(desktopHdc);
#endif

    if (!m_glrc) {
      m_glrc = wglCreateContext(hdc);
      if (sharedGlrc)
        wglShareLists(sharedGlrc, m_glrc);
    }

    if (!m_glrc) {
      ReleaseDC(hwnd, hdc);
      return false;
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
