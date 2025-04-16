// LAF OS Library
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SURFACE_H_INCLUDED
#define OS_SURFACE_H_INCLUDED
#pragma once

#include "base/string.h"
#include "gfx/clip.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "os/color_space.h"
#include "os/paint.h"
#include "os/ref.h"
#include "os/sampling.h"
#include "os/surface_format.h"

#include <string>

namespace gfx {
class Matrix;
class Path;
class Region;
} // namespace gfx

namespace os {

class ColorSpace;
struct Sampling;
class Surface;
class SurfaceLock;
using SurfaceRef = Ref<Surface>;

class Surface : public RefCount {
public:
  enum class ColorChannelsOrder { RGB, BGR };

  static ColorChannelsOrder getNativeColorChannelsOrder();

  virtual ~Surface() {}
  virtual int width() const = 0;
  virtual int height() const = 0;
  gfx::Rect bounds() const { return gfx::Rect(0, 0, width(), height()); }
  virtual const ColorSpaceRef& colorSpace() const = 0;
  virtual bool isDirectToScreen() const = 0;

  // Call if you are not going to modify the pixels of this surface
  // in the future. E.g. useful for sprite sheets/texture atlases.
  virtual void setImmutable() = 0;

  virtual int getSaveCount() const = 0;
  virtual gfx::Rect getClipBounds() const = 0;
  virtual void saveClip() = 0;
  virtual void restoreClip() = 0;
  virtual bool clipRect(const gfx::Rect& rc) = 0;
  virtual void clipPath(const gfx::Path& path) = 0;
  virtual void clipRegion(const gfx::Region& region) = 0;

  virtual void save() = 0;
  virtual void concat(const gfx::Matrix& matrix) = 0;
  virtual void setMatrix(const gfx::Matrix& matrix) = 0;
  virtual void resetMatrix() = 0;
  virtual void restore() = 0;
  virtual gfx::Matrix matrix() const = 0;

  virtual void lock() = 0;
  virtual void unlock() = 0;

  virtual void clear() = 0;

  virtual uint8_t* getData(int x, int y) const = 0;
  virtual void getFormat(SurfaceFormatData* formatData) const = 0;

  virtual gfx::Color getPixel(int x, int y) const = 0;
  virtual void putPixel(gfx::Color color, int x, int y) = 0;

  virtual void drawLine(float x0, float y0, float x1, float y1, const os::Paint& paint) = 0;

  void drawLine(const int x0, const int y0, const int x1, const int y1, const os::Paint& paint)
  {
    drawLine(float(x0), float(y0), float(x1), float(y1), paint);
  }

  void drawLine(const gfx::PointF& a, const gfx::PointF& b, const os::Paint& paint)
  {
    drawLine(float(a.x), float(a.y), float(b.x), float(b.y), paint);
  }

  void drawLine(const gfx::Point& a, const gfx::Point& b, const os::Paint& paint)
  {
    drawLine(float(a.x), float(a.y), float(b.x), float(b.y), paint);
  }

  virtual void drawRect(const gfx::RectF& rc, const os::Paint& paint) = 0;

  void drawRect(const gfx::Rect& rc, const os::Paint& paint)
  {
    drawRect(gfx::RectF(float(rc.x), float(rc.y), float(rc.w), float(rc.h)), paint);
  }

  virtual void drawCircle(float cx, float cy, float radius, const os::Paint& paint) = 0;

  void drawCircle(const gfx::PointF& center, float radius, const os::Paint& paint)
  {
    drawCircle(float(center.x), float(center.y), radius, paint);
  }

  void drawCircle(const gfx::Point& center, int radius, const os::Paint& paint)
  {
    drawCircle(float(center.x), float(center.y), float(radius), paint);
  }

  virtual void drawPath(const gfx::Path& path, const os::Paint& paint) = 0;

  virtual void blitTo(Surface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height)
    const = 0;
  virtual void scrollTo(const gfx::Rect& rc, int dx, int dy) = 0;
  // TODO merge all these functions expoing a SkPaint-like structure
  virtual void drawSurface(const Surface* src, int dstx, int dsty) = 0;
  virtual void drawSurface(const Surface* src,
                           const gfx::Rect& srcRect,
                           const gfx::Rect& dstRect,
                           const os::Sampling& sampling = os::Sampling(),
                           const os::Paint* paint = nullptr) = 0;
  virtual void drawRgbaSurface(const Surface* src, int dstx, int dsty) = 0;
  virtual void drawRgbaSurface(const Surface* src,
                               int srcx,
                               int srcy,
                               int dstx,
                               int dsty,
                               int width,
                               int height) = 0;
  virtual void drawColoredRgbaSurface(const Surface* src,
                                      gfx::Color fg,
                                      gfx::Color bg,
                                      const gfx::Clip& clip) = 0;
  virtual void drawSurfaceNine(os::Surface* surface,
                               const gfx::Rect& src,
                               const gfx::Rect& center,
                               const gfx::Rect& dst,
                               bool drawCenter,
                               const os::Paint* paint) = 0;

  // Returns the same surface if scaleFactor == 1.0 or a new scaled
  // surface.
  [[nodiscard]]
  virtual SurfaceRef applyScale(float scaleFactor, const Sampling& sampling = {}) = 0;

  virtual void* nativeHandle() = 0;
};

class SurfaceLock {
public:
  SurfaceLock(Surface* surface) : m_surface(surface) { m_surface->lock(); }
  ~SurfaceLock() { m_surface->unlock(); }

private:
  Surface* m_surface;
};

} // namespace os

#endif
