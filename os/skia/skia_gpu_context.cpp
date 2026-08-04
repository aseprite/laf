// LAF OS Library
// Copyright (C) 2022-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/log.h"
#include "os/skia/skia_gpu_context.h"

#include "os/skia/skia_surface.h"

#if SK_SUPPORT_GPU

  #include "include/core/SkSurface.h"
  #include "include/gpu/ganesh/GrBackendSurface.h"
  #include "include/gpu/ganesh/SkSurfaceGanesh.h"
  #include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
  #include "include/gpu/ganesh/gl/GrGLDirectContext.h"
  #include "src/gpu/ganesh/gl/GrGLDefines.h"

namespace os {

SkiaGpuContext::SkiaGpuContext(std::unique_ptr<GpuContext> subCtx) : m_subCtx(std::move(subCtx))
{
}

bool SkiaGpuContext::attachGL()
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

void SkiaGpuContext::detachGL()
{
  if (m_grCtx) {
    m_grCtx->abandonContext();
    m_grCtx.reset(nullptr);
  }
  m_glInterfaces.reset(nullptr);
}

bool SkiaGpuContext::isValid()
{
  return m_grCtx && m_subCtx->isValid();
}

bool SkiaGpuContext::makeContext(Window* window, GpuContext* shared)
{
  bool result = m_subCtx->makeContext(window, shared);
  if (result)
    result = attachGL();
  return result;
}

void SkiaGpuContext::destroyContext()
{
  detachGL();
  m_subCtx->destroyContext();
}

void SkiaGpuContext::makeCurrent(Window* window)
{
  m_subCtx->makeCurrent(window);
}

void SkiaGpuContext::swapBuffers(Window* window)
{
  m_grCtx->flushAndSubmit();
  m_subCtx->swapBuffers(window);
}

void SkiaGpuContext::flush()
{
  m_glInterfaces->fFunctions.fFlush();
}

RenderTarget SkiaGpuContext::makeRenderTarget(const gfx::Size& size,
                                              const int scale,
                                              const os::ColorSpaceRef& cs)
{
  if (!m_glInterfaces || !m_glInterfaces->fFunctions.fGetIntegerv)
    return {};

  // Create of a SkSurface (fbSurface) connected to the OpenGL
  // framebuffer.
  RenderTarget renderTarget;

  GrGLint buffer;
  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_FRAMEBUFFER_BINDING, &buffer);

  GrGLFramebufferInfo info;
  info.fFBOID = (GrGLuint)buffer;
  info.fFormat = GR_GL_RGBA8;

  GrGLint stencil = 0;
  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_STENCIL_BITS, &stencil);

  GrBackendRenderTarget target = GrBackendRenderTargets::MakeGL(size.w, size.h, 0, stencil, info);

  renderTarget.fbSurface = os::make_ref<SkiaSurface>(
    SkSurfaces::WrapBackendRenderTarget(m_grCtx.get(),
                                        target,
                                        kBottomLeft_GrSurfaceOrigin,
                                        kRGBA_8888_SkColorType,
                                        ((SkiaColorSpace*)cs.get())->skColorSpace(),
                                        nullptr));

  if (!renderTarget.fbSurface)
    return renderTarget;

  if (scale == 1 && renderTarget.fbSurface) {
    renderTarget.surface = renderTarget.fbSurface;
  }
  else {
    SkImageInfo info = SkImageInfo::Make(std::max(1, size.w / scale),
                                         std::max(1, size.h / scale),
                                         kN32_SkColorType,
                                         kOpaque_SkAlphaType,
                                         ((SkiaColorSpace*)cs.get())->skColorSpace());

    renderTarget.surface = os::make_ref<SkiaSurface>(
      SkSurfaces::RenderTarget(m_grCtx.get(), skgpu::Budgeted::kNo, info, 0, nullptr));
  }

  return renderTarget;
}

} // namespace os

#endif
