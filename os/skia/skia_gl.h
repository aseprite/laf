// LAF OS Library
// Copyright (C) 2022-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_GL_INCLUDED
#define OS_SKIA_SKIA_GL_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "gfx/size.h"

#if SK_SUPPORT_GPU
  #include "include/core/SkColorSpace.h"
  #include "include/core/SkRefCnt.h"
  #include "include/core/SkSurface.h"
  #include "include/gpu/ganesh/GrDirectContext.h"
  #include "include/gpu/ganesh/gl/GrGLInterface.h"
#endif

namespace os {

#if SK_SUPPORT_GPU

class SkiaGL {
public:
  SkiaGL();

  bool hasCtx() const { return m_grCtx != nullptr; }
  GrDirectContext* grCtx() const { return m_grCtx.get(); };
  const GrGLInterface* glInterfaces() const { return m_glInterfaces.get(); };

  bool attachGL();
  void detachGL();

  bool createRenderTarget(const gfx::Size& size, const int scale, sk_sp<SkColorSpace> colorSpace);

  sk_sp<SkSurface> backbufferSurface() const { return m_backbufferSurface; }
  sk_sp<SkSurface> surface() const { return m_surface; }

private:
  sk_sp<const GrGLInterface> m_glInterfaces;
  sk_sp<GrDirectContext> m_grCtx;
  sk_sp<SkSurface> m_backbufferSurface;
  sk_sp<SkSurface> m_surface;

  DISABLE_COPYING(SkiaGL);
};

#else // !SK_SUPPORT_GPU

class SkiaGL {};

#endif

} // namespace os

#endif
