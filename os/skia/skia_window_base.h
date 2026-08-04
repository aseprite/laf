// LAF OS Library
// Copyright (C) 2021-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_WINDOW_BASE_INCLUDED
#define OS_SKIA_SKIA_WINDOW_BASE_INCLUDED
#pragma once

#include "os/event.h"
#include "os/event_queue.h"
#include "os/skia/skia_gpu_context.h"
#include "os/skia/skia_surface.h"
#include "os/skia/skia_system.h"
#include "os/system.h"
#include "os/window.h"

#include "include/core/SkCanvas.h"

namespace os {

template<typename T>
class SkiaWindowBase : public T {
public:
  using Base = SkiaWindowBase<T>;

  template<typename... Args>
  SkiaWindowBase(Args&&... args)
    : T(std::forward<Args&&>(args)...)
    , m_initialized(false)
    , m_surface(new SkiaSurface)
    , m_colorSpace(nullptr)
  {
  }

  ~SkiaWindowBase()
  {
#if SK_SUPPORT_GPU
    destroyGpuContext();
#endif
  }

  void initColorSpace()
  {
    // Needed on macOS because WindowOSX::colorSpace() needs the
    // m_nsWindow created, and that happens after
    // WindowOSX::createWindow() is called.
    m_colorSpace = T::colorSpace();
  }

  bool isInitialized() const { return m_initialized; }

  void resetSkiaSurface()
  {
    if (m_surface)
      m_surface.reset();

    resizeSkiaSurface(this->clientSize());
  }

  void resizeSkiaSurface(const gfx::Size& size)
  {
    if (!m_initialized)
      return;

    gfx::Size newSize(size.w / this->scale(), size.h / this->scale());
    newSize.w = std::max(1, newSize.w);
    newSize.h = std::max(1, newSize.h);

    if (m_initialized && m_surface && m_surface->width() == newSize.w &&
        m_surface->height() == newSize.h) {
      return;
    }

    m_backend = Backend::NONE;
    m_surface.reset();

#if SK_SUPPORT_GPU
    // GPU-accelerated surface
    if (m_preferGpuAcceleration) {
      auto* gpuCtx = gpuContext();
      if (gpuCtx) {
        if (!gpuCtx->isValid()) {
          auto sys = System::instance();
          GpuContext* shared = (sys ? sys->gpuContext() : nullptr);

          gpuCtx->makeContext(this, shared);
  #if 0 // TODO set shared GpuContext
          if (sys && !shared && gpuCtx->isValid())
            sys->setGpuContext(gpuCtx);
  #endif
        }

        if (gpuCtx->isValid()) {
          gpuCtx->makeCurrent(this);

          m_renderTarget = gpuCtx->makeRenderTarget(size, this->scale(), colorSpace());
          if (m_renderTarget.surface) {
            m_surface = m_renderTarget.surface;
            m_backend = Backend::GL;
          }
        }
      }
    }
    else {
      m_renderTarget = {};
    }
#endif // SK_SUPPORT_GPU

    // Raster surface
    if (!m_surface) {
      m_surface = make_ref<SkiaSurface>();
      if (T::isTransparent())
        static_cast<SkiaSurface*>(m_surface.get())->createRgba(newSize.w, newSize.h, colorSpace());
      else
        static_cast<SkiaSurface*>(m_surface.get())->create(newSize.w, newSize.h, colorSpace());
    }
  }

  // Returns the main surface to draw into this window.
  // You must not dispose this surface.
  Surface* surface() override { return m_surface.get(); }

  // Overrides the colorSpace() method to return the cached/stored
  // color space in this instance (instead of asking for the color
  // space to the screen as T::colorSpace() should do).
  os::ColorSpaceRef colorSpace() const override
  {
    if (m_colorSpace)
      return m_colorSpace;
    return T::colorSpace();
  }

  void setColorSpace(const os::ColorSpaceRef& colorSpace) override
  {
    m_colorSpace = colorSpace;

    if (m_surface)
      resetSkiaSurface();

    // Generate the resizing window event to redraw everything.
    // TODO we could create a new event like Event::ColorSpaceChange,
    // but the result would be the same, the window must be re-painted.
    Event ev;
    ev.setType(Event::ResizeWindow);
    ev.setWindow(AddRef(this));
    os::queue_event(ev);
  }

  void makeCurrent() override
  {
#if SK_SUPPORT_GPU
    if (m_backend == Backend::NONE)
      return;

    auto* gpuCtx = gpuContext();
    if (gpuCtx && gpuCtx->isValid())
      gpuCtx->makeCurrent(this);
#endif
  }

  void swapBuffers() override
  {
#if SK_SUPPORT_GPU
    if (m_backend == Backend::NONE || !m_renderTarget.fbSurface)
      return;

    auto* gpuCtx = gpuContext();
    if (!gpuCtx || !gpuCtx->isValid())
      return;

    auto surface = static_cast<SkiaSurface*>(this->surface());
    if (!surface)
      return;

    // Draw the small (unscaled) surface to the framebuffer surface
    // scaling it to the this->scale() factor.
    if (m_renderTarget.fbSurface != m_renderTarget.surface) {
      SkSamplingOptions sampling;
      SkPaint paint;

      SkCanvas* dstCanvas =
        static_cast<SkiaSurface*>(m_renderTarget.fbSurface.get())->skSurface()->getCanvas();
      dstCanvas->save();
      dstCanvas->scale(SkScalar(this->scale()), SkScalar(this->scale()));
      static_cast<SkiaSurface*>(m_renderTarget.surface.get())
        ->skSurface()
        ->draw(dstCanvas, 0.0, 0.0, sampling, &paint);
      dstCanvas->restore();
    }

    gpuCtx->flush();
    gpuCtx->swapBuffers(this);
#endif // SK_SUPPORT_GPU
  }

  bool gpuAcceleration() const override
  {
#if SK_SUPPORT_GPU
    return (m_backend == Backend::GL && m_gpuCtx && m_gpuCtx->isValid());
#else
    return false;
#endif
  }

  void setGpuAcceleration(bool state) override
  {
#if SK_SUPPORT_GPU
    if (state) {
      if (!gpuContext())
        makeGpuContext();
    }
#endif

    m_preferGpuAcceleration = state;
    resetSkiaSurface();

    T::setGpuAcceleration(state);
  }

protected:
  void initializeSurface()
  {
    m_initialized = true;
    resetSkiaSurface();
  }

  void onResize(const gfx::Size& sz) override
  {
    resizeSkiaSurface(sz);

    if (System::instance()->handleWindowResize &&
        // Check that the surface is created to avoid a call to
        // handleWindowResize() with an empty surface (or null
        // SkiaSurface::m_canvas) when the window is being created.
        isInitialized()) {
      System::instance()->handleWindowResize(this);
    }
    else {
      Event ev;
      ev.setType(Event::ResizeWindow);
      ev.setWindow(AddRef(this));
      queue_event(ev);
    }
  }

  enum class Backend {
    NONE,
#if SK_SUPPORT_GPU
    GL,
#endif
  };

  Backend backend() const { return m_backend; }

#if SK_SUPPORT_GPU
  SkiaGpuContext* gpuContext() override { return m_gpuCtx.get(); }
#endif

private:
#if SK_SUPPORT_GPU
  void makeGpuContext()
  {
    if (!m_gpuCtx)
      m_gpuCtx = std::make_unique<SkiaGpuContext>(System::instance()->makePlatformGpuContext());
  }

  void destroyGpuContext()
  {
    if (m_gpuCtx && m_gpuCtx->isValid()) {
      auto* sys = System::rawInstance();
      if (sys && sys->gpuContext() == m_gpuCtx.get())
        sys->setGpuContext(nullptr);

      m_gpuCtx->destroyContext();
      m_gpuCtx.reset();
    }
  }
#endif

  bool m_preferGpuAcceleration = false;
  Backend m_backend = Backend::NONE;
  // Flag used to avoid accessing to an invalid m_surface in the first
  // SkiaWindow::resize() call when the window is created (as the
  // window is created, it send a first resize event.)
  bool m_initialized;
  RenderTarget m_renderTarget;
  SurfaceRef m_surface;
  os::ColorSpaceRef m_colorSpace;
#if SK_SUPPORT_GPU
  std::unique_ptr<SkiaGpuContext> m_gpuCtx;
#endif
};

} // namespace os

#endif
