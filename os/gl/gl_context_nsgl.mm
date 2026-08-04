// LAF OS Library
// Copyright (C) 2022-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/gl/gl_context_nsgl.h"

#include "os/window.h"

#include <vector>

#include <Cocoa/Cocoa.h>
#include <OpenGL/gl.h>

namespace os {

GLContextNSGL::GLContextNSGL()
{
}

GLContextNSGL::~GLContextNSGL()
{
  destroyContext();
}

void GLContextNSGL::setView(id view)
{
  [view setWantsBestResolutionOpenGLSurface:YES];
  m_view = view;

  if (m_nsgl)
    [m_nsgl setView:m_view];
}

bool GLContextNSGL::isValid()
{
  return m_nsgl != nil;
}

bool GLContextNSGL::makeContext(Window* window, GpuContext* shared)
{
  // set up pixel format
  std::vector<NSOpenGLPixelFormatAttribute> attr;
  attr.push_back(NSOpenGLPFADoubleBuffer);
  attr.push_back(NSOpenGLPFAAccelerated);
  attr.push_back(NSOpenGLPFAClosestPolicy);
  attr.push_back(NSOpenGLPFABackingStore);
  attr.push_back(NSOpenGLPFAOpenGLProfile);
  attr.push_back(NSOpenGLProfileVersion3_2Core);
  attr.push_back(NSOpenGLPFAColorSize);
  attr.push_back(24);
  attr.push_back(NSOpenGLPFAAlphaSize);
  attr.push_back(8);
  attr.push_back(NSOpenGLPFADepthSize);
  attr.push_back(0);
  attr.push_back(NSOpenGLPFAStencilSize);
  attr.push_back(8);
  attr.push_back(NSOpenGLPFASampleBuffers);
  attr.push_back(0);
  attr.push_back(0);

  auto nsPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:&attr[0]];
  if (!nsPixelFormat)
    return false;

  // TODO NSOpenGLContext shared

  m_nsgl = [[NSOpenGLContext alloc] initWithFormat:nsPixelFormat shareContext:nil];
  if (!m_nsgl)
    return false;

  GLint swapInterval = 0; // disable vsync
  [m_nsgl setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

  if (m_view)
    [m_nsgl setView:m_view];

  makeCurrent(window);
  return true;
}

void GLContextNSGL::destroyContext()
{
  m_nsgl = nil;
}

void GLContextNSGL::makeCurrent(Window* window)
{
  [m_nsgl makeCurrentContext];

  const gfx::Size clientSize = window->clientSize();

  if (m_lastClientSize != clientSize) {
    m_lastClientSize = clientSize;
    [m_nsgl update];
  }

  glViewport(0, 0, clientSize.w, clientSize.h);
}

void GLContextNSGL::swapBuffers(Window* window)
{
  [m_nsgl flushBuffer];
}

} // namespace os
