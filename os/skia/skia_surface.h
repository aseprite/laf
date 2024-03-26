// LAF OS Library
// Copyright (c) 2018-2024  Igara Studio S.A.
// Copyright (c) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_SURFACE_INCLUDED
#define OS_SKIA_SKIA_SURFACE_INCLUDED
#pragma once

#include "base/exception.h"
#include "gfx/clip.h"
#include "gfx/matrix.h"
#include "os/common/generic_surface.h"
#include "os/common/sprite_sheet_font.h"
#include "os/skia/skia_color_space.h"
#include "os/surface_format.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkColorType.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"

#include <atomic>

namespace os {

class SkiaSurface final : public Surface {
public:
  SkiaSurface();
  SkiaSurface(const sk_sp<SkSurface>& surface);
  ~SkiaSurface();

  void create(int width, int height, const os::ColorSpaceRef& cs);
  void create(int width, int height,  const SurfaceFormatData& sf, const unsigned char* data);
  void createRgba(int width, int height, const os::ColorSpaceRef& cs);
  void destroy();

  void flush() const;
  void flushAndSubmit() const;

  // Surface impl
  int width() const override;
  int height() const override;
  const ColorSpaceRef& colorSpace() const override;
  bool isDirectToScreen() const override;
  void setImmutable() override;
  int getSaveCount() const override;
  gfx::Rect getClipBounds() const override;
  void saveClip() override;
  void restoreClip() override;
  bool clipRect(const gfx::Rect& rc) override;
  void clipPath(const gfx::Path& path) override;
  void save() override;
  void concat(const gfx::Matrix& matrix) override;
  void setMatrix(const gfx::Matrix& matrix) override;
  void resetMatrix() override;
  void restore() override;
  gfx::Matrix matrix() const override;
  void lock() override;
  void unlock() override;
  void applyScale(int scaleFactor) override;

  void* nativeHandle() override;

  void clear() override;
  uint8_t* getData(int x, int y) const override;
  void getFormat(SurfaceFormatData* formatData) const override;

  gfx::Color getPixel(int x, int y) const override;
  void putPixel(gfx::Color color, int x, int y) override;

  void drawLine(const float x0, const float y0,
                const float x1, const float y1,
                const Paint& paint) override;

  void drawRect(const gfx::RectF& rc,
                const Paint& paint) override;

  void drawCircle(const float cx, const float cy,
                  const float radius,
                  const Paint& paint) override;

  void drawPath(const gfx::Path& path,
                const Paint& paint) override;

  void blitTo(Surface* _dst, int srcx, int srcy, int dstx, int dsty, int width, int height) const override;
  void scrollTo(const gfx::Rect& rc, int dx, int dy) override;
  void drawSurface(const Surface* src, int dstx, int dsty) override;
  void drawSurface(const Surface* src,
                   const gfx::Rect& srcRect,
                   const gfx::Rect& dstRect,
                   const Sampling& sampling,
                   const os::Paint* paint) override;
  void drawRgbaSurface(const Surface* src, int dstx, int dsty) override;
  void drawRgbaSurface(const Surface* src, int srcx, int srcy, int dstx, int dsty, int w, int h) override;
  void drawColoredRgbaSurface(const Surface* src, gfx::Color fg, gfx::Color bg, const gfx::Clip& clipbase) override;
  void drawSurfaceNine(os::Surface* surface,
                       const gfx::Rect& src,
                       const gfx::Rect& _center,
                       const gfx::Rect& dst,
                       const bool drawCenter,
                       const os::Paint* paint) override;

  bool isValid() const {
    return !m_bitmap.isNull();
  }

  SkBitmap& bitmap() {
    ASSERT(!m_bitmap.isNull());
    return m_bitmap;
  }
  SkCanvas& canvas() { return *m_canvas; }

  void swapBitmap(SkBitmap& other);

  static SurfaceRef loadSurface(const char* filename);

private:
  static SkColorType deductSkColorType(const os::SurfaceFormatData& sf);
  static os::PixelAlpha asPixelAlpha(SkAlphaType at);
  static SkAlphaType asSkAlphaType(os::PixelAlpha pa);
  void skDrawSurface(
    const Surface* src,
    const gfx::Clip& clip,
    const SkSamplingOptions& sampling,
    const SkPaint& paint);
  void skDrawSurface(
    const Surface* src,
    const gfx::Rect& srcRect,
    const gfx::Rect& dstRect,
    const SkSamplingOptions& sampling,
    const SkPaint& paint);
  void skDrawSurface(
    const SkiaSurface* src,
    const SkRect& srcRect,
    const SkRect& dstRect,
    const SkSamplingOptions& sampling,
    const SkPaint& paint);

#if SK_SUPPORT_GPU
  const SkImage* getOrCreateTextureImage() const;
  bool uploadBitmapAsTexture() const;
#endif

  sk_sp<SkColorSpace> skColorSpace() const {
    if (m_colorSpace)
      return static_cast<SkiaColorSpace*>(m_colorSpace.get())->skColorSpace();
    return nullptr;
  }

  SkBitmap m_bitmap;
#if SK_SUPPORT_GPU
  mutable sk_sp<SkImage> m_image;
#endif
  sk_sp<SkSurface> m_surface;
  ColorSpaceRef m_colorSpace;
  SkCanvas* m_canvas;
  SkPaint m_paint;
  std::atomic<int> m_lock;

};

} // namespace os

#endif
