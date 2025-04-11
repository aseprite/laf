// LAF OS Library
// Copyright (c) 2018-2025  Igara Studio S.A.
// Copyright (c) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/skia/skia_surface.h"

#include "base/file_handle.h"
#include "gfx/path.h"
#include "gfx/region.h"
#include "os/skia/skia_helpers.h"
#include "os/surface_format.h"
#include "os/system.h"

#include "include/codec/SkCodec.h"
#include "include/core/SkAlphaType.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixelRef.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSize.h"
#include "include/core/SkStream.h"
#include "src/core/SkColorData.h"

#if SK_SUPPORT_GPU
  #include "include/gpu/ganesh/GrBackendSurface.h"
  #include "include/gpu/ganesh/GrDirectContext.h"
  #include "include/gpu/ganesh/SkImageGanesh.h"
  #include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
  #include "src/image/SkSurface_Raster.h"
#endif

#include <memory>
#include <stddef.h>

namespace os {

static PixelAlpha from_skia(const SkAlphaType at)
{
  switch (at) {
    case SkAlphaType::kOpaque_SkAlphaType:   return PixelAlpha::kOpaque;
    case SkAlphaType::kPremul_SkAlphaType:   return PixelAlpha::kPremultiplied;
    case SkAlphaType::kUnpremul_SkAlphaType: return PixelAlpha::kStraight;
    default:
      // Invalid alpha type
      ASSERT(false);
      return PixelAlpha::kPremultiplied;
  }
}

static SkCanvas::SrcRectConstraint to_constraint(const Paint* paint)
{
  if (paint && paint->srcEdges() == Paint::SrcEdges::Fast)
    return SkCanvas::kFast_SrcRectConstraint;
  return SkCanvas::kStrict_SrcRectConstraint;
}

// static
Surface::ColorChannelsOrder Surface::getNativeColorChannelsOrder()
{
  // There are two possible values by Skia (at the moment)
  static_assert(kN32_SkColorType == kRGBA_8888_SkColorType ||
                kN32_SkColorType == kBGRA_8888_SkColorType);
  if constexpr (kN32_SkColorType == kRGBA_8888_SkColorType)
    return ColorChannelsOrder::RGB;
  if constexpr (kN32_SkColorType == kBGRA_8888_SkColorType)
    return ColorChannelsOrder::BGR;
}

SkiaSurface::SkiaSurface() : m_surface(nullptr), m_colorSpace(nullptr), m_canvas(nullptr), m_lock(0)
{
}

SkiaSurface::SkiaSurface(const sk_sp<SkSurface>& surface)
  : m_surface(surface)
  , m_colorSpace(nullptr)
  , m_canvas(nullptr)
  , m_lock(0)
{
  ASSERT(m_surface);
  if (m_surface)
    m_canvas = m_surface->getCanvas();
}

SkiaSurface::~SkiaSurface()
{
  ASSERT(m_lock == 0);
  destroy();
}

void SkiaSurface::create(int width, int height, const os::ColorSpaceRef& cs)
{
  destroy();

  ASSERT(!m_surface)
  ASSERT(width > 0);
  ASSERT(height > 0);

  m_colorSpace = cs;

  SkBitmap bmp;
  if (!bmp.tryAllocPixels(SkImageInfo::MakeN32(width, height, kOpaque_SkAlphaType, skColorSpace())))
    throw base::Exception("Cannot create Skia surface");

  bmp.eraseColor(SK_ColorTRANSPARENT);
  swapBitmap(bmp);
}

void SkiaSurface::createRgba(int width, int height, const os::ColorSpaceRef& cs)
{
  destroy();

  ASSERT(!m_surface);
  ASSERT(width > 0);
  ASSERT(height > 0);

  m_colorSpace = cs;

  SkBitmap bmp;
  if (!bmp.tryAllocPixels(SkImageInfo::MakeN32Premul(width, height, skColorSpace())))
    throw base::Exception("Cannot create Skia surface");

  bmp.eraseColor(SK_ColorTRANSPARENT);
  swapBitmap(bmp);
}

void SkiaSurface::createWithBitmap(SkBitmap&& bmp, const os::ColorSpaceRef& cs)
{
  m_colorSpace = cs;
  swapBitmap(bmp);
}

void SkiaSurface::destroy()
{
  if (!m_surface) {
    delete m_canvas;
    m_canvas = nullptr;
  }
}

void SkiaSurface::flush() const
{
  // TODO replace with GrDirectContext::flush()
  // if (m_canvas)
  //   m_canvas->flush();
}

void SkiaSurface::flushAndSubmit() const
{
  // TODO replace with GrDirectContext::flushAndSubmit()
  // if (m_surface)
  //   m_surface->flushAndSubmit();
}

int SkiaSurface::width() const
{
  if (m_surface)
    return m_surface->width();
  return m_bitmap.width();
}

int SkiaSurface::height() const
{
  if (m_surface)
    return m_surface->height();
  return m_bitmap.height();
}

const ColorSpaceRef& SkiaSurface::colorSpace() const
{
  return m_colorSpace;
}

bool SkiaSurface::isDirectToScreen() const
{
  return false;
}

void SkiaSurface::setImmutable()
{
  if (!m_bitmap.isNull())
    m_bitmap.setImmutable();
}

int SkiaSurface::getSaveCount() const
{
  return m_canvas->getSaveCount();
}

gfx::Rect SkiaSurface::getClipBounds() const
{
  SkIRect rc;
  if (m_canvas->getDeviceClipBounds(&rc))
    return gfx::Rect(rc.x(), rc.y(), rc.width(), rc.height());
  return gfx::Rect();
}

void SkiaSurface::saveClip()
{
  m_canvas->save();
}

void SkiaSurface::restoreClip()
{
  m_canvas->restore();
}

bool SkiaSurface::clipRect(const gfx::Rect& rc)
{
  m_canvas->clipRect(SkRect::Make(to_skia(rc)));
  return !m_canvas->isClipEmpty();
}

void SkiaSurface::clipPath(const gfx::Path& path)
{
  m_canvas->clipPath(path.skPath());
}

void SkiaSurface::clipRegion(const gfx::Region& region)
{
  m_canvas->clipRegion(region.skRegion());
}

void SkiaSurface::save()
{
  m_canvas->save();
}

void SkiaSurface::concat(const gfx::Matrix& matrix)
{
  m_canvas->concat(matrix.skMatrix());
}

void SkiaSurface::setMatrix(const gfx::Matrix& matrix)
{
  m_canvas->setMatrix(matrix.skMatrix());
}

void SkiaSurface::resetMatrix()
{
  m_canvas->resetMatrix();
}

void SkiaSurface::restore()
{
  m_canvas->restore();
}

gfx::Matrix SkiaSurface::matrix() const
{
  return m_canvas->getTotalMatrix();
}

void SkiaSurface::lock()
{
  ASSERT(m_lock >= 0);
  if (m_lock++ == 0) {
    // TODO add mutex!
    // m_bitmap is always locked
  }
}

void SkiaSurface::unlock()
{
  ASSERT(m_lock > 0);
  if (--m_lock == 0) {
    // m_bitmap is always locked
  }
}

SurfaceRef SkiaSurface::applyScale(float scaleFactor, const Sampling& sampling)
{
  if (scaleFactor == 1.0f)
    return AddRef(this);

  ASSERT(!m_surface);

  SkBitmap result;
  if (!result.tryAllocPixels(m_bitmap.info().makeWH(width() * scaleFactor, height() * scaleFactor)))
    throw base::Exception("Cannot create temporary Skia surface to change scale");

  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrc);

  SkCanvas canvas(result);
  SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(0, 0, width(), height()));
  SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(0, 0, result.width(), result.height()));

  SkSamplingOptions skSampling;
  to_skia(sampling, skSampling);

  canvas.drawImageRect(SkImages::RasterFromPixmap(m_bitmap.pixmap(), nullptr, nullptr),
                       srcRect,
                       dstRect,
                       skSampling,
                       &paint,
                       SkCanvas::kStrict_SrcRectConstraint);

  auto resultSurface = base::make_ref<SkiaSurface>();
  resultSurface->createWithBitmap(std::move(result), m_colorSpace);
  return resultSurface;
}

void* SkiaSurface::nativeHandle()
{
  return (void*)this;
}

void SkiaSurface::clear()
{
  m_canvas->clear(0);
}

uint8_t* SkiaSurface::getData(int x, int y) const
{
  if (SkBitmap* bitmap = getBitmap())
    return (uint8_t*)bitmap->getAddr32(x, y);

  return nullptr;
}

SkBitmap* SkiaSurface::getBitmap() const
{
  if (!m_bitmap.isNull())
    return const_cast<SkBitmap*>(&m_bitmap);

  // The SkSurface_Raster::skBitmap() member function was added by us
  // in our own Skia branch.
  if (auto* base = static_cast<SkSurface_Base*>(m_surface.get())) {
    if (base->type() == SkSurface_Base::Type::kRaster) {
      auto* raster = static_cast<SkSurface_Raster*>(base);
      return &raster->skBitmap();
    }
  }

  return nullptr;
}

void SkiaSurface::getFormat(SurfaceFormatData* formatData) const
{
  SkBitmap* bitmap = getBitmap();
  if (!bitmap)
    return;

  formatData->format = kRgbaSurfaceFormat;
  formatData->bitsPerPixel = 8 * bitmap->bytesPerPixel();
  formatData->pixelAlpha = from_skia(bitmap->alphaType());

  switch (bitmap->colorType()) {
    case kRGB_565_SkColorType:
      formatData->redShift = SK_R16_SHIFT;
      formatData->greenShift = SK_G16_SHIFT;
      formatData->blueShift = SK_B16_SHIFT;
      formatData->alphaShift = 0;
      formatData->redMask = SK_R16_MASK;
      formatData->greenMask = SK_G16_MASK;
      formatData->blueMask = SK_B16_MASK;
      formatData->alphaMask = 0;
      break;
    case kARGB_4444_SkColorType:
      formatData->redShift = SK_R4444_SHIFT;
      formatData->greenShift = SK_G4444_SHIFT;
      formatData->blueShift = SK_B4444_SHIFT;
      formatData->alphaShift = SK_A4444_SHIFT;
      formatData->redMask = (15 << SK_R4444_SHIFT);
      formatData->greenMask = (15 << SK_G4444_SHIFT);
      formatData->blueMask = (15 << SK_B4444_SHIFT);
      formatData->alphaMask = (15 << SK_A4444_SHIFT);
      break;
    case kRGBA_8888_SkColorType:
      formatData->redShift = SK_RGBA_R32_SHIFT;
      formatData->greenShift = SK_RGBA_G32_SHIFT;
      formatData->blueShift = SK_RGBA_B32_SHIFT;
      formatData->alphaShift = SK_RGBA_A32_SHIFT;
      formatData->redMask = (255 << SK_RGBA_R32_SHIFT);
      formatData->greenMask = (255 << SK_RGBA_G32_SHIFT);
      formatData->blueMask = (255 << SK_RGBA_B32_SHIFT);
      formatData->alphaMask = (255 << SK_RGBA_A32_SHIFT);
      break;
    case kBGRA_8888_SkColorType:
      formatData->redShift = SK_BGRA_R32_SHIFT;
      formatData->greenShift = SK_BGRA_G32_SHIFT;
      formatData->blueShift = SK_BGRA_B32_SHIFT;
      formatData->alphaShift = SK_BGRA_A32_SHIFT;
      formatData->redMask = (255 << SK_BGRA_R32_SHIFT);
      formatData->greenMask = (255 << SK_BGRA_G32_SHIFT);
      formatData->blueMask = (255 << SK_BGRA_B32_SHIFT);
      formatData->alphaMask = (255 << SK_BGRA_A32_SHIFT);
      break;
    default:
      formatData->redShift = 0;
      formatData->greenShift = 0;
      formatData->blueShift = 0;
      formatData->alphaShift = 0;
      formatData->redMask = 0;
      formatData->greenMask = 0;
      formatData->blueMask = 0;
      formatData->alphaMask = 0;
      break;
  }
}

gfx::Color SkiaSurface::getPixel(int x, int y) const
{
  // Clip input to avoid crash on SkBitmap::getColor()
  if (x < 0 || y < 0 || x >= width() || y >= height())
    return 0;

  SkColor c = 0;

  if (m_surface) {
    SkImageInfo dstInfo = SkImageInfo::MakeN32Premul(1, 1, skColorSpace());
    uint32_t dstPixels;
    if (m_canvas->readPixels(dstInfo, &dstPixels, 4, x, y))
      c = dstPixels;
  }
  else
    c = m_bitmap.getColor(x, y);

  return gfx::rgba(SkColorGetR(c), SkColorGetG(c), SkColorGetB(c), SkColorGetA(c));
}

void SkiaSurface::putPixel(gfx::Color color, int x, int y)
{
  if (m_surface) {
    m_paint.setColor4f(to_skia4f(color), skColorSpace().get());
    m_canvas->drawPoint(SkIntToScalar(x), SkIntToScalar(y), m_paint);
  }
  else {
    // TODO Find a better way to put a pixel in the same color space
    //      as the internal SkPixmap (as Skia expects a sRGB color
    //      in SkBitmap::erase())
#if 1
    auto r = SkIRect::MakeXYWH(x, y, 1, 1);
    const_cast<SkPixmap&>(m_bitmap.pixmap()).erase(to_skia4f(color), &r);
#else
    // Doesn't work as SkBitmap::erase() expects an sRGB color (and
    // the color is should be in the same color space as this
    // surface, so there is no conversion).
    m_bitmap.erase(to_skia(color), SkIRect::MakeXYWH(x, y, 1, 1));
#endif
  }
}

void SkiaSurface::drawLine(const float x0,
                           const float y0,
                           const float x1,
                           const float y1,
                           const Paint& paint)
{
  m_canvas->drawLine(x0, y0, x1, y1, paint.skPaint());
}

void SkiaSurface::drawRect(const gfx::RectF& rc, const Paint& paint)
{
  if (rc.isEmpty())
    return;

  if (paint.style() == Paint::Style::Stroke)
    m_canvas->drawRect(to_skia_fix(rc), paint.skPaint());
  else
    m_canvas->drawRect(to_skia(rc), paint.skPaint());
}

void SkiaSurface::drawCircle(const float cx, const float cy, const float radius, const Paint& paint)
{
  m_canvas->drawCircle(cx, cy, radius, paint.skPaint());
}

void SkiaSurface::drawPath(const gfx::Path& path, const Paint& paint)
{
  m_canvas->drawPath(path.skPath(), paint.skPaint());
}

void SkiaSurface::blitTo(Surface* _dst,
                         int srcx,
                         int srcy,
                         int dstx,
                         int dsty,
                         int width,
                         int height) const
{
  auto dst = static_cast<SkiaSurface*>(_dst);

  SkRect srcRect = SkRect::MakeXYWH(srcx, srcy, width, height);
  SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(dstx, dsty, width, height));

  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrc);

#if SK_SUPPORT_GPU
  if (auto srcImage = getOrCreateTextureImage()) {
    dst->m_canvas->drawImageRect(srcImage,
                                 srcRect,
                                 dstRect,
                                 SkSamplingOptions(),
                                 &paint,
                                 SkCanvas::kStrict_SrcRectConstraint);
    return;
  }
#endif

  if (!m_bitmap.empty()) {
    dst->m_canvas->drawImageRect(SkImages::RasterFromPixmap(m_bitmap.pixmap(), nullptr, nullptr),
                                 srcRect,
                                 dstRect,
                                 SkSamplingOptions(),
                                 &paint,
                                 SkCanvas::kStrict_SrcRectConstraint);
  }
  else {
    sk_sp<SkImage> snapshot = m_surface->makeImageSnapshot(srcRect.round());
    srcRect.offsetTo(0, 0);
    dst->m_canvas->drawImageRect(snapshot,
                                 srcRect,
                                 dstRect,
                                 SkSamplingOptions(),
                                 &paint,
                                 SkCanvas::kStrict_SrcRectConstraint);
  }
}

void SkiaSurface::scrollTo(const gfx::Rect& rc, int dx, int dy)
{
  int w = width();
  int h = height();
  gfx::Clip clip(rc.x + dx, rc.y + dy, rc);
  if (!clip.clip(w, h, w, h))
    return;

  if (m_surface) {
    SurfaceLock lock(this);
    blitTo(this, clip.src.x, clip.src.y, clip.dst.x, clip.dst.y, clip.size.w, clip.size.h);
    return;
  }

  int bytesPerPixel = m_bitmap.bytesPerPixel();
  int rowBytes = (int)m_bitmap.rowBytes();
  int rowDelta;

  if (dy > 0) {
    clip.src.y += clip.size.h - 1;
    clip.dst.y += clip.size.h - 1;
    rowDelta = -rowBytes;
  }
  else
    rowDelta = rowBytes;

  char* dst = (char*)m_bitmap.getPixels();
  const char* src = dst;
  dst += rowBytes * clip.dst.y + bytesPerPixel * clip.dst.x;
  src += rowBytes * clip.src.y + bytesPerPixel * clip.src.x;
  w = bytesPerPixel * clip.size.w;
  h = clip.size.h;

  while (--h >= 0) {
    memmove(dst, src, w);
    dst += rowDelta;
    src += rowDelta;
  }

  m_bitmap.notifyPixelsChanged();
}

void SkiaSurface::drawSurface(const Surface* src, int dstx, int dsty)
{
  gfx::Clip clip(dstx, dsty, 0, 0, src->width(), src->height());
  // Don't call clip.clip() and left the clipping to the Skia library
  // (mainly because Skia knows how to handle clipping even when a
  // matrix is set).

  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrc);
  skDrawSurface(src, clip, SkSamplingOptions(), paint, SkCanvas::kStrict_SrcRectConstraint);
}

void SkiaSurface::drawSurface(const Surface* src,
                              const gfx::Rect& srcRect,
                              const gfx::Rect& dstRect,
                              const Sampling& sampling,
                              const os::Paint* paint)
{
  SkPaint skSrcPaint;
  skSrcPaint.setBlendMode(SkBlendMode::kSrc);

  SkSamplingOptions skSampling;
  to_skia(sampling, skSampling);

  skDrawSurface(src,
                srcRect,
                dstRect,
                skSampling,
                (paint ? paint->skPaint() : skSrcPaint),
                to_constraint(paint));
}

void SkiaSurface::drawRgbaSurface(const Surface* src, int dstx, int dsty)
{
  gfx::Clip clip(dstx, dsty, 0, 0, src->width(), src->height());

  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrcOver);
  skDrawSurface(src, clip, SkSamplingOptions(), paint, SkCanvas::kStrict_SrcRectConstraint);
}

void SkiaSurface::drawRgbaSurface(const Surface* src,
                                  int srcx,
                                  int srcy,
                                  int dstx,
                                  int dsty,
                                  int w,
                                  int h)
{
  gfx::Clip clip(dstx, dsty, srcx, srcy, w, h);

  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrcOver);
  skDrawSurface(src, clip, SkSamplingOptions(), paint, SkCanvas::kStrict_SrcRectConstraint);
}

void SkiaSurface::drawColoredRgbaSurface(const Surface* src,
                                         gfx::Color fg,
                                         gfx::Color bg,
                                         const gfx::Clip& clipbase)
{
  gfx::Clip clip(clipbase);

  SkRect srcRect = SkRect::Make(
    SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
  SkRect dstRect = SkRect::Make(
    SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrcOver);

  if (gfx::geta(bg) > 0) {
    SkPaint paint;
    paint.setColor4f(to_skia4f(bg), skColorSpace().get());
    paint.setStyle(SkPaint::kFill_Style);
    m_canvas->drawRect(dstRect, paint);
  }

  sk_sp<SkColorFilter> colorFilter(SkColorFilters::Blend(to_skia(fg), SkBlendMode::kSrcIn));
  paint.setColorFilter(colorFilter);

  skDrawSurface((SkiaSurface*)src,
                srcRect,
                dstRect,
                SkSamplingOptions(),
                paint,
                SkCanvas::kStrict_SrcRectConstraint);
}

void SkiaSurface::drawSurfaceNine(os::Surface* surface,
                                  const gfx::Rect& src,
                                  const gfx::Rect& center,
                                  const gfx::Rect& dst,
                                  const bool drawCenter,
                                  const os::Paint* paint)
{
  SkIRect srcRect = SkIRect::MakeXYWH(src.x, src.y, src.w, src.h);
  SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(dst.x, dst.y, dst.w, dst.h));

  SkPaint skPaint;
  skPaint.setBlendMode(SkBlendMode::kSrcOver);
  if (paint && paint->color() != gfx::ColorNone) {
    sk_sp<SkColorFilter> colorFilter(
      SkColorFilters::Blend(to_skia(paint->color()), SkBlendMode::kSrcIn));
    skPaint.setColorFilter(colorFilter);
  }

  int xdivs[2] = { src.x + center.x, src.x + center.x2() };
  int ydivs[2] = { src.y + center.y, src.y + center.y2() };
  SkCanvas::Lattice::RectType rectTypes[9] = { SkCanvas::Lattice::kDefault,
                                               SkCanvas::Lattice::kDefault,
                                               SkCanvas::Lattice::kDefault,
                                               SkCanvas::Lattice::kDefault,
                                               (drawCenter ? SkCanvas::Lattice::kDefault :
                                                             SkCanvas::Lattice::kTransparent),
                                               SkCanvas::Lattice::kDefault,
                                               SkCanvas::Lattice::kDefault,
                                               SkCanvas::Lattice::kDefault,
                                               SkCanvas::Lattice::kDefault };

  // Without left side
  if (center.x == 0) {
    srcRect.fLeft -= 1;
    dstRect.fLeft -= 1;
    rectTypes[0] = rectTypes[3] = rectTypes[6] = SkCanvas::Lattice::kTransparent;
  }

  // Without right side
  if (center.x2() == src.w) {
    srcRect.fRight += 1;
    dstRect.fRight += 1;
    rectTypes[2] = rectTypes[5] = rectTypes[8] = SkCanvas::Lattice::kTransparent;
  }

  // Without top side
  if (center.y == 0) {
    srcRect.fTop -= 1;
    dstRect.fTop -= 1;
    rectTypes[0] = rectTypes[1] = rectTypes[2] = SkCanvas::Lattice::kTransparent;
  }

  // Without bottom side
  if (center.y2() == src.h) {
    srcRect.fBottom += 1;
    srcRect.fBottom += 1;
    rectTypes[6] = rectTypes[7] = rectTypes[8] = SkCanvas::Lattice::kTransparent;
  }

  SkCanvas::Lattice lattice;
  lattice.fXDivs = xdivs;
  lattice.fYDivs = ydivs;
  lattice.fRectTypes = rectTypes;
  lattice.fXCount = 2;
  lattice.fYCount = 2;
  lattice.fBounds = &srcRect;
  lattice.fColors = nullptr;

#if SK_SUPPORT_GPU
  if (auto srcImage = ((SkiaSurface*)surface)->getOrCreateTextureImage()) {
    m_canvas->drawImageLattice(srcImage, lattice, dstRect, SkFilterMode::kNearest, &skPaint);
    return;
  }
#endif

  auto image =
    SkImages::RasterFromPixmap(((SkiaSurface*)surface)->m_bitmap.pixmap(), nullptr, nullptr);
  m_canvas->drawImageLattice(image.get(), lattice, dstRect, SkFilterMode::kNearest, &skPaint);
}

void SkiaSurface::swapBitmap(SkBitmap& other)
{
  ASSERT(!m_surface);
  m_bitmap.swap(other);
  delete m_canvas;
  m_canvas = new SkCanvas(m_bitmap);
}

// static
Ref<Surface> SkiaSurface::loadSurface(const char* filename)
{
  FILE* f = base::open_file_raw(filename, "rb");
  if (!f)
    return nullptr;

  std::unique_ptr<SkCodec> codec(
    SkCodec::MakeFromStream(std::unique_ptr<SkFILEStream>(new SkFILEStream(f))));
  if (!codec)
    return nullptr;

  SkImageInfo info =
    codec->getInfo().makeColorType(kN32_SkColorType).makeAlphaType(kPremul_SkAlphaType);
  SkBitmap bm;
  if (!bm.tryAllocPixels(info))
    return nullptr;

  const SkCodec::Result r = codec->getPixels(info, bm.getPixels(), bm.rowBytes());
  if (r != SkCodec::kSuccess)
    return nullptr;

  auto sur = make_ref<SkiaSurface>();
  sur->swapBitmap(bm);
  return sur;
}

void SkiaSurface::skDrawSurface(const Surface* src,
                                const gfx::Clip& clip,
                                const SkSamplingOptions& sampling,
                                const SkPaint& paint,
                                const SkCanvas::SrcRectConstraint constraint)
{
  skDrawSurface(static_cast<const SkiaSurface*>(src),
                SkRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h),
                SkRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h),
                sampling,
                paint,
                constraint);
}

void SkiaSurface::skDrawSurface(const Surface* src,
                                const gfx::Rect& srcRect,
                                const gfx::Rect& dstRect,
                                const SkSamplingOptions& sampling,
                                const SkPaint& paint,
                                const SkCanvas::SrcRectConstraint constraint)
{
  skDrawSurface(static_cast<const SkiaSurface*>(src),
                SkRect::MakeXYWH(srcRect.x, srcRect.y, srcRect.w, srcRect.h),
                SkRect::MakeXYWH(dstRect.x, dstRect.y, dstRect.w, dstRect.h),
                sampling,
                paint,
                constraint);
}

void SkiaSurface::skDrawSurface(const SkiaSurface* src,
                                const SkRect& srcRect,
                                const SkRect& dstRect,
                                const SkSamplingOptions& sampling,
                                const SkPaint& paint,
                                const SkCanvas::SrcRectConstraint constraint)
{
#if SK_SUPPORT_GPU
  src->flush();
  if (auto srcImage = src->getOrCreateTextureImage()) {
    m_canvas->drawImageRect(srcImage, srcRect, dstRect, sampling, &paint, constraint);
    return;
  }
#endif

  m_canvas->drawImageRect(SkImages::RasterFromPixmap(src->m_bitmap.pixmap(), nullptr, nullptr),
                          srcRect,
                          dstRect,
                          sampling,
                          &paint,
                          constraint);
}

#if SK_SUPPORT_GPU

const SkImage* SkiaSurface::getOrCreateTextureImage() const
{
  // TODO use the GrDirectContext of the specific os::Window
  auto win = System::instance()->defaultWindow();
  if (!win || !win->sk_grCtx())
    return nullptr;

  // Invalidate the cached texture if the bitmap pixels were modified.
  if (m_cachedGen && m_cachedGen != m_bitmap.getGenerationID())
    m_image.reset();

  if (m_image && m_image->isValid((SkRecorder*)win->sk_grCtx()))
    return m_image.get();
  if (uploadBitmapAsTexture() && m_image && m_image->isValid((SkRecorder*)win->sk_grCtx()))
    return m_image.get();
  return nullptr;
}

bool SkiaSurface::uploadBitmapAsTexture() const
{
  SkImageInfo ii = m_bitmap.info();
  sk_sp<SkImage> image = m_bitmap.asImage();

  // TODO use the window of this surface
  Window* win = System::instance()->defaultWindow();

  GrBackendTexture texture;
  SkImages::BackendTextureReleaseProc proc;
  SkImages::GetBackendTextureFromImage(win->sk_grCtx(), image, &texture, &proc);

  m_image = SkImages::BorrowTextureFrom(win->sk_grCtx(),
                                        texture,
                                        kTopLeft_GrSurfaceOrigin,
                                        ii.colorType(),
                                        ii.alphaType(),
                                        nullptr);

  if (m_image)
    m_cachedGen = m_bitmap.getGenerationID();

  return (m_image != nullptr);
}

#endif // SK_SUPPORT_GPU

} // namespace os
