// LAF OS Library
// Copyright (C) 2022-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/log.h"
#include "os/skia/skia_gl.h"

#if SK_SUPPORT_GPU

  #include "include/core/SkSurface.h"
  #include "include/gpu/ganesh/GrBackendSurface.h"
  #include "include/gpu/ganesh/SkSurfaceGanesh.h"
  #include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
  #include "include/gpu/ganesh/gl/GrGLDirectContext.h"
  #include "src/gpu/ganesh/gl/GrGLDefines.h"

namespace os {

SkiaGL::SkiaGL()
{
}

bool SkiaGL::attachGL()
{
  if (m_grCtx)
    return true;

  try {
    m_glInterfaces = GrGLMakeNativeInterface();
    if (!m_glInterfaces) {
      LOG(ERROR, "OS: Cannot get native GL interface\n");
      detachGL();
      return false;
    }

    m_grCtx = GrDirectContexts::MakeGL(m_glInterfaces);
    if (!m_grCtx) {
      LOG(ERROR, "OS: Cannot create GrContext\n");
      detachGL();
      return false;
    }

    LOG("OS: Using OpenGL backend\n");
  }
  catch (const std::exception& ex) {
    LOG(ERROR, "OS: Cannot create GL context: %s\n", ex.what());
    detachGL();
    return false;
  }
  return true;
}

void SkiaGL::detachGL()
{
  if (m_grCtx) {
    m_grCtx->abandonContext();
    m_grCtx.reset(nullptr);
  }
  m_glInterfaces.reset(nullptr);
}

bool SkiaGL::createRenderTarget(const gfx::Size& size,
                                const int scale,
                                sk_sp<SkColorSpace> colorSpace)
{
  // Create of a SkSurface (m_backbufferSurface) connected to the
  // OpenGL framebuffer.

  GrGLint buffer;
  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_FRAMEBUFFER_BINDING, &buffer);

  GrGLFramebufferInfo info;
  info.fFBOID = (GrGLuint)buffer;
  info.fFormat = GR_GL_RGBA8;
  // SK_API bool GrBackendRenderTargets::GetGLFramebufferInfo(const GrBackendRenderTarget&,
  // GrGLFramebufferInfo*);

  GrGLint stencil = 0;
  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_STENCIL_BITS, &stencil);

  // GrBackendRenderTarget target(size.w, size.h, 0, stencil, info);
  GrBackendRenderTarget target = GrBackendRenderTargets::MakeGL(size.w, size.h, 0, stencil, info);

  m_surface.reset(nullptr);
  m_backbufferSurface = SkSurfaces::WrapBackendRenderTarget(m_grCtx.get(),
                                                            target,
                                                            kBottomLeft_GrSurfaceOrigin,
                                                            kRGBA_8888_SkColorType,
                                                            colorSpace,
                                                            nullptr);

  if (!m_backbufferSurface)
    return false;

  if (scale == 1 && m_backbufferSurface) {
    m_surface = m_backbufferSurface;
  }
  else {
    SkImageInfo info = SkImageInfo::Make(std::max(1, size.w / scale),
                                         std::max(1, size.h / scale),
                                         kN32_SkColorType,
                                         kOpaque_SkAlphaType,
                                         colorSpace);

    m_surface = SkSurfaces::RenderTarget(m_grCtx.get(), skgpu::Budgeted::kNo, info, 0, nullptr);
  }

  return true;
}

} // namespace os

#endif
