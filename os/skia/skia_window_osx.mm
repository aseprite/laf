// LAF OS Library
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

// #define DEBUG_UPDATE_RECTS

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/skia/skia_window_osx.h"

#include "base/log.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/gl/gl_context_nsgl.h"
#include "os/osx/event_queue.h"
#include "os/osx/view.h"
#include "os/skia/skia_color_space.h"
#include "os/skia/skia_surface.h"
#include "os/skia/skia_window_osx.h"
#include "os/system.h"

#include "include/utils/mac/SkCGUtils.h"

#if SK_SUPPORT_GPU
  #include <Cocoa/Cocoa.h>
  #include <OpenGL/gl.h>
#endif

#include <algorithm>
#include <iostream>

namespace os {

SkiaWindowOSX::SkiaWindowOSX(const WindowSpec& spec)
{
#if SK_SUPPORT_GPU
  m_glCtx = std::make_unique<GLContextNSGL>();
#endif

  m_closing = false;
  createWindow(spec);
  initColorSpace();

#if SK_SUPPORT_GPU
  auto nsgl = (GLContextNSGL*)m_glCtx.get();
  nsgl->setView([m_nsWindow contentView]);
#endif
}

SkiaWindowOSX::~SkiaWindowOSX()
{
  destroyWindow();
}

void SkiaWindowOSX::setFullscreen(bool state)
{
  // Do not call toggleFullScreen in the middle of other toggleFullScreen
  if (m_resizingCount > 0)
    return;

  WindowOSX::setFullscreen(state);
}

void SkiaWindowOSX::invalidateRegion(const gfx::Region& rgn)
{
  switch (backend()) {
    case Backend::NONE:
      @autoreleasepool {
        gfx::Rect bounds = rgn.bounds(); // TODO use only the region?
        int scale = this->scale();
        NSView* view = m_nsWindow.contentView;

        // We add an extra pixel (in scale units) at the top, to avoid
        // some problems non-refreshing invalidated areas.
        //
        // TODO investigate why this is really needed, e.g. on
        // Aseprite, when we move a ui::Display inside other
        // ui::Display, or in the Preview window to update the brush
        // preview in real-time.
        [view setNeedsDisplayInRect:NSMakeRect(bounds.x * scale,
                                               view.frame.size.height - bounds.y2() * scale - scale,
                                               bounds.w * scale,
                                               bounds.h * scale + scale)];

#if 0 // Do not refresh immediately. Note: This might be required
      // for debugging purposes in some scenarios, but now this is
      // not required in release mode.
      //
      // TODO maybe in a future we could add an Display::update()
      //      or Display::refresh() member function
        [view displayIfNeeded];
#endif
      }
      break;

#if SK_SUPPORT_GPU

    case Backend::GL: m_gl.glInterfaces()->fFunctions.fFlush(); break;

#endif
  }
}

void SkiaWindowOSX::setTextInput(bool state, const gfx::Point& screenCaretPos)
{
  ViewOSX* view = (ViewOSX*)m_nsWindow.contentView;
  [view setTranslateDeadKeys:(state ? YES : NO)];
}

void SkiaWindowOSX::onClose()
{
  m_closing = true;
}

void SkiaWindowOSX::onResize(const gfx::Size& size)
{
  resizeSkiaSurface(size);
}

void SkiaWindowOSX::onDrawRect(const gfx::Rect& rect)
{
  if (m_nsWindow.contentView.inLiveResize) {
    SystemRef system = System::instance();
    if (system && system->handleWindowResize)
      system->handleWindowResize(this);
  }

  switch (backend()) {
    case Backend::NONE: paintGC(rect); break;

#if SK_SUPPORT_GPU
    case Backend::GL: m_gl.glInterfaces()->fFunctions.fFlush(); break;
#endif
  }
}

void SkiaWindowOSX::onWindowChanged()
{
#if SK_SUPPORT_GPU
  if (m_glCtx) {
    auto nsgl = (GLContextNSGL*)m_glCtx.get();
    nsgl->setView([m_nsWindow contentView]);
  }
#endif
}

void SkiaWindowOSX::onStartResizing()
{
  if (++m_resizingCount > 1)
    return;
}

void SkiaWindowOSX::onResizing(gfx::Size& size)
{
  resizeSkiaSurface(size);

  SystemRef system = System::instance();
  ASSERT(system);
  if (system && system->handleWindowResize)
    system->handleWindowResize(this);
}

void SkiaWindowOSX::onEndResizing()
{
  if (--m_resizingCount > 0)
    return;

  // Generate the resizing display event for the user.
  SystemRef system = System::instance();
  ASSERT(system);
  if (system && system->handleWindowResize) {
    system->handleWindowResize(this);
  }
  else {
    Event ev;
    ev.setType(Event::ResizeWindow);
    ev.setWindow(AddRef(this));
    os::queue_event(ev);
  }
}

void SkiaWindowOSX::onChangeBackingProperties()
{
  if (m_nsWindow)
    setColorSpace(colorSpace());
}

void SkiaWindowOSX::paintGC(const gfx::RectF& rect)
{
  if (!this->isInitialized())
    return;

  if (rect.isEmpty())
    return;

  NSRect viewBounds = m_nsWindow.contentView.bounds;
  float scale = this->scale();

  SkiaSurface* surface = static_cast<SkiaSurface*>(this->surface());
  if (!surface->isValid())
    return;

  const bool transparent = isTransparent();
  const SkBitmap& origBitmap = surface->bitmap();

  SkBitmap bitmap;
  if (scale == 1.0f) {
    // Create a subset to draw on the view
    if (!origBitmap.extractSubset(
          &bitmap,
          SkIRect::MakeXYWH(rect.x, (viewBounds.size.height - (rect.y + rect.h)), rect.w, rect.h)))
      return;
  }
  else {
    // Create a bitmap to draw the original one scaled. This is
    // faster than doing the scaling directly in
    // CGContextDrawImage(). This avoid a slow path where the
    // internal macOS argb32_image_mark_RGB32() function is called
    // (which is a performance hit).
    if (!bitmap.tryAllocN32Pixels(rect.w, rect.h, !transparent))
      return;

    if (transparent)
      bitmap.eraseColor(0);

    SkCanvas canvas(bitmap);
    canvas.drawImageRect(SkImages::RasterFromPixmap(origBitmap.pixmap(), nullptr, nullptr),
                         SkRect::MakeXYWH(rect.x / scale,
                                          (viewBounds.size.height - (rect.y + rect.h)) / scale,
                                          rect.w / scale,
                                          rect.h / scale),
                         SkRect::MakeXYWH(0, 0, rect.w, rect.h),
                         SkSamplingOptions(),
                         nullptr,
                         SkCanvas::kStrict_SrcRectConstraint);
  }

  @autoreleasepool {
    NSGraphicsContext* gc = [NSGraphicsContext currentContext];
    CGContextRef cg = (CGContextRef)[gc graphicsPort];
    // TODO we can be in other displays (non-main display)
    CGColorSpaceRef colorSpace = CGDisplayCopyColorSpace(CGMainDisplayID());
    CGImageRef img = SkCreateCGImageRefWithColorspace(bitmap, colorSpace);
    if (img) {
      CGRect r =
        CGRectMake(viewBounds.origin.x + rect.x, viewBounds.origin.y + rect.y, rect.w, rect.h);

      CGContextSaveGState(cg);
      CGContextSetInterpolationQuality(cg, kCGInterpolationNone);
      CGContextDrawImage(cg, r, img);
#ifdef DEBUG_UPDATE_RECTS
      {
        static int i = 0;
        i = (i + 1) % 8;
        CGContextSetRGBStrokeColor(cg,
                                   (i & 1 ? 1.0f : 0.0f),
                                   (i & 2 ? 1.0f : 0.0f),
                                   (i & 4 ? 1.0f : 0.0f),
                                   1.0f);
        CGContextStrokeRectWithWidth(cg, r, 2.0f);
      }
#endif
      CGContextRestoreGState(cg);
      CGImageRelease(img);
    }
    CGColorSpaceRelease(colorSpace);
  }
}

} // namespace os
