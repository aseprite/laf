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
  m_subCtx->swapBuffers(window);
}

void SkiaGpuContext::flush()
{
  m_grCtx->flushAndSubmit();
}

SurfaceRef SkiaGpuContext::makeOnscreenRenderTarget(const gfx::Size& size,
                                                    const os::ColorSpaceRef& cs)
{
  GrGLuint buffer = 0; // On-screen canvas has FBO = 0
  GrGLint samples = 0;
  GrGLint stencil = 8;
  GrGLFramebufferInfo info;
  info.fFBOID = buffer;
  info.fFormat = GR_GL_RGBA8;

  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_SAMPLES, &samples);
  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_STENCIL_BITS, &stencil);

  GrBackendRenderTarget target;
  target = GrBackendRenderTargets::MakeGL(size.w, size.h, samples, stencil, info);
  if (!target.isValid())
    return {};

  auto rt = SkSurfaces::WrapBackendRenderTarget(
    m_grCtx.get(),
    target,
    kBottomLeft_GrSurfaceOrigin,
    kRGBA_8888_SkColorType,
    (cs ? ((SkiaColorSpace*)cs.get())->skColorSpace() : nullptr),
    nullptr);
  if (!rt)
    return {};

  return os::make_ref<SkiaSurface>(rt);
}

SurfaceRef SkiaGpuContext::makeOffscreenRenderTarget(const gfx::Size& size,
                                                     const os::ColorSpaceRef& cs)
{
  GrGLint samples = 0;
  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_SAMPLES, &samples);

  SkImageInfo info = SkImageInfo::Make(
    size.w,
    size.h,
    kN32_SkColorType,
    kOpaque_SkAlphaType,
    (cs ? ((SkiaColorSpace*)cs.get())->skColorSpace() : nullptr));

  auto rt = SkSurfaces::RenderTarget(m_grCtx.get(), skgpu::Budgeted::kNo, info, samples, nullptr);
  if (!rt)
    return {};

  return os::make_ref<SkiaSurface>(rt);
}

} // namespace os

#endif
