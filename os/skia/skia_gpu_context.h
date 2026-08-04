// LAF OS Library
// Copyright (C) 2022-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_GPU_INCLUDED
#define OS_SKIA_SKIA_GPU_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "gfx/size.h"
#include "os/gpu_context.h"

#if SK_SUPPORT_GPU
  #include "include/core/SkColorSpace.h"
  #include "include/core/SkRefCnt.h"
  #include "include/core/SkSurface.h"
  #include "include/gpu/ganesh/GrDirectContext.h"
  #include "include/gpu/ganesh/gl/GrGLInterface.h"
#endif

namespace os {

#if SK_SUPPORT_GPU

class SkiaGpuContext : public GpuContext {
public:
  SkiaGpuContext(std::unique_ptr<GpuContext> subCtx);

  // GpuContext impl
  bool isValid() override;
  bool makeContext(Window* window, GpuContext* shared) override;
  void destroyContext() override;
  void* nativeHandle() override { return (m_subCtx ? m_subCtx->nativeHandle() : nullptr); }

  void makeCurrent(Window* window) override;
  void swapBuffers(Window* window) override;
  RenderTarget makeRenderTarget(const gfx::Size& size,
                                int scale,
                                const os::ColorSpaceRef& cs) override;
  void flush() override;

  GrDirectContext* grCtx() const { return m_grCtx.get(); }

private:
  bool attachGL();
  void detachGL();

  std::unique_ptr<GpuContext> m_subCtx;
  sk_sp<const GrGLInterface> m_glInterfaces;
  sk_sp<GrDirectContext> m_grCtx;

  DISABLE_COPYING(SkiaGpuContext);
};

#endif // SK_SUPPORT_GPU

} // namespace os

#endif
