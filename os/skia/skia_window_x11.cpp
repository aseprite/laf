// LAF OS Library
// Copyright (C) 2020-present  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/skia/skia_window_x11.h"

#include "gfx/size.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/gl/gl_context_glx.h"
#include "os/skia/skia_surface.h"
#include "os/skia/skia_window.h"
#include "os/system.h"
#include "os/x11/x11.h"

#include "include/core/SkBitmap.h"

namespace os {

namespace {

bool convert_skia_bitmap_to_ximage(const SkBitmap& bitmap, XImage& image)
{
  memset(&image, 0, sizeof(image));
  int bpp = 8 * bitmap.bytesPerPixel();
  image.width = bitmap.width();
  image.height = bitmap.height();
  image.format = ZPixmap;
  image.data = (char*)bitmap.getPixels();
  image.byte_order = LSBFirst;
  image.bitmap_unit = bpp;
  image.bitmap_bit_order = LSBFirst;
  image.bitmap_pad = bpp;
  image.depth = (bitmap.alphaType() == kPremul_SkAlphaType ? 32 : 24);
  image.bytes_per_line = bitmap.rowBytes() - 4 * bitmap.width();
  image.bits_per_pixel = bpp;

  return (XInitImage(&image) ? true : false);
}

} // anonymous namespace

SkiaWindowX11::SkiaWindowX11(const WindowSpec& spec) : Base(X11::instance()->display(), spec)
{
#if SK_SUPPORT_GPU
  m_glCtx = std::make_unique<GLContextGLX>(x11display(), x11window());
#endif
  initColorSpace();
}

void SkiaWindowX11::onPaint(const gfx::Rect& rc)
{
#if SK_SUPPORT_GPU
  if (backend() == Backend::GL) {
    m_gl.glInterfaces()->fFunctions.fFlush();
    return;
  }
#endif

  auto surface = static_cast<SkiaSurface*>(this->surface());
  const SkBitmap& bitmap = surface->bitmap();

  int scale = this->scale();
  if (scale == 1) {
    XImage image;
    if (convert_skia_bitmap_to_ximage(bitmap, image)) {
      XPutImage(x11display(), x11window(), gc(), &image, rc.x, rc.y, rc.x, rc.y, rc.w, rc.h);
    }
  }
  else {
    SkBitmap scaled;
    const SkImageInfo info =
      SkImageInfo::Make(rc.w, rc.h, bitmap.info().colorType(), bitmap.info().alphaType());

    // Increase m_buffer for "scaled" pixels if needed.
    const size_t rowBytes = info.minRowBytes();
    const size_t requiredSize = info.computeByteSize(rowBytes);
    if (requiredSize > m_buffer.size())
      m_buffer.resize(requiredSize);

    // Clear the buffer to avoid showing the previous rendered content
    // displaced.
    std::fill(m_buffer.begin(), m_buffer.end(), 0);

    if (scaled.installPixels(info, (void*)m_buffer.data(), rowBytes)) {
      SkPaint paint;
      paint.setBlendMode(SkBlendMode::kSrc);

      SkCanvas canvas(scaled);
      SkRect srcRect = SkRect::Make(
        SkIRect::MakeXYWH(rc.x / scale, rc.y / scale, rc.w / scale, rc.h / scale));
      SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(0, 0, rc.w, rc.h));
      canvas.drawImageRect(SkImages::RasterFromPixmap(bitmap.pixmap(), nullptr, nullptr),
                           srcRect,
                           dstRect,
                           SkSamplingOptions(),
                           &paint,
                           SkCanvas::kStrict_SrcRectConstraint);

      XImage image;
      if (convert_skia_bitmap_to_ximage(scaled, image)) {
        XPutImage(x11display(), x11window(), gc(), &image, 0, 0, rc.x, rc.y, rc.w, rc.h);
      }
    }
  }
}

} // namespace os
