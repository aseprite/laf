// LAF OS Library
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/skia/skia_window_win.h"

#include "base/log.h"
#include "gfx/region.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/skia/skia_window.h"
#include "os/system.h"

#include "os/win/window_dde.h"
#include <windows.h>

#include <algorithm>
#include <iostream>

namespace os {

SkiaWindowWin::SkiaWindowWin(const WindowSpec& spec) : SkiaWindowBase<WindowWin>(spec)
{
  initColorSpace();
}

void SkiaWindowWin::onPaint(HDC hdc)
{
  switch (backend()) {
    case Backend::NONE: paintHDC(hdc); break;

#if SK_SUPPORT_GPU
    case Backend::GL: {
      auto* gpuCtx = gpuContext();
      if (gpuCtx && gpuCtx->isValid())
        gpuCtx->makeCurrent(this);
      // gpuCtx->flush();
      break;
    }
#endif
  }
}

void SkiaWindowWin::invalidateRegion(const gfx::Region& rgn)
{
  if (!isTransparent())
    return WindowWin::invalidateRegion(rgn);

  // Special logic for transparent (WS_EX_LAYERED) windows: we call
  // UpdateLayeredWindowIndirect() because we want to present the RGBA
  // surface as the window surface with alpha per pixel.

  SkiaSurface* surface = static_cast<SkiaSurface*>(this->surface());
  ASSERT(surface);

  if (!surface || !surface->isValid())
    return;

  const SkBitmap& bitmap = surface->bitmap();
  const int w = bitmap.width();
  const int h = bitmap.height();
  const int s = scale();
  const int sw = bitmap.width() * s;
  const int sh = bitmap.height() * s;

  HWND hwnd = (HWND)nativeHandle();
  HDC hdc = GetDC(nullptr);
  HBITMAP hbmpScaled = CreateCompatibleBitmap(hdc, sw, sh);
  HBITMAP hbmp = CreateBitmap(w, h, 1, 32, (void*)bitmap.getPixels());
  HDC srcHdcScaled = CreateCompatibleDC(hdc);
  HDC srcHdc = CreateCompatibleDC(hdc);
  SelectObject(srcHdcScaled, hbmpScaled);
  SelectObject(srcHdc, hbmp);

  BLENDFUNCTION bf;
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = 255;
  bf.AlphaFormat = AC_SRC_ALPHA;

  AlphaBlend(srcHdcScaled, 0, 0, sw, sh, srcHdc, 0, 0, w, h, bf);

  const gfx::Rect rect = frame();
  const POINT dstPoint = { rect.x, rect.y };
  const SIZE dstSize = { rect.w, rect.h };
  POINT srcPos = { 0, 0 };

  const gfx::Rect dirtyBounds = rgn.bounds();
  const RECT dirty = { s * dirtyBounds.x,
                       s * dirtyBounds.y,
                       s * dirtyBounds.x2(),
                       s * dirtyBounds.y2() };

  UPDATELAYEREDWINDOWINFO ulwi;
  memset(&ulwi, 0, sizeof(ulwi));
  ulwi.cbSize = sizeof(ulwi);
  ulwi.hdcDst = hdc;
  ulwi.pptDst = &dstPoint;
  ulwi.psize = &dstSize;
  ulwi.hdcSrc = srcHdcScaled;
  ulwi.pptSrc = &srcPos;
  ulwi.pblend = &bf;
  ulwi.dwFlags = ULW_ALPHA;
  ulwi.prcDirty = &dirty;

  UpdateLayeredWindowIndirect(hwnd, &ulwi);

  ReleaseDC(nullptr, hdc);
  DeleteObject(hbmpScaled);
  DeleteObject(hbmp);
  DeleteDC(srcHdcScaled);
  DeleteDC(srcHdc);
}

void SkiaWindowWin::paintHDC(HDC hdc)
{
  if (isTransparent()) {
    // In transparent windows we don't handle WM_PAINT messages, but
    // call UpdateLayeredWindowIndirect() directly from
    // invalidateRegion().
    return;
  }

  SkiaSurface* surface = static_cast<SkiaSurface*>(this->surface());
  ASSERT(surface);

  // It looks like the surface can be nullptr here from a WM_PAINT
  // message (issue received from a dmp file).
  if (!surface || !surface->isValid())
    return;

  const SkBitmap& bitmap = surface->bitmap();

  BITMAPINFO bmi;
  memset(&bmi, 0, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = bitmap.width();
  bmi.bmiHeader.biHeight = -bitmap.height();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;

  ASSERT(bitmap.width() * bitmap.bytesPerPixel() == bitmap.rowBytes());

  int ret = StretchDIBits(hdc,
                          0,
                          0,
                          bitmap.width() * scale(),
                          bitmap.height() * scale(),
                          0,
                          0,
                          bitmap.width(),
                          bitmap.height(),
                          bitmap.getPixels(),
                          &bmi,
                          DIB_RGB_COLORS,
                          SRCCOPY);
  (void)ret;
}

void SkiaWindowWin::onStartResizing()
{
  m_resizing = true;
}

void SkiaWindowWin::onEndResizing()
{
  m_resizing = false;

  Event ev;
  ev.setType(Event::ResizeWindow);
  ev.setWindow(AddRef(this));
  queue_event(ev);
}

void SkiaWindowWin::onChangeColorSpace()
{
  // Set the monitor color space (bypass SkiaWindowBase::m_colorSpace).
  setColorSpace(WindowWin::colorSpace());
}

} // namespace os
