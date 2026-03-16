// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/skia/skia_window_ios.h"

#include "base/log.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/skia/skia_color_space.h"
#include "os/skia/skia_surface.h"
#include "os/system.h"

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#include "os/ios/view.h"

namespace os {

SkiaWindowIOS::SkiaWindowIOS(const WindowSpec& spec)
{
  m_closing = false;
  createWindow(spec);

  // Set back-reference so the view can send events to this window
  AsepriteView* view = asepriteView();
  if (view)
    view.aseWindow = this;

  initColorSpace();
  initializeSurface();
}

SkiaWindowIOS::~SkiaWindowIOS()
{
  destroyWindow();
}

void SkiaWindowIOS::invalidateRegion(const gfx::Region& rgn)
{
  if (!this->isInitialized())
    return;

  SkiaSurface* surface = static_cast<SkiaSurface*>(this->surface());
  if (!surface || !surface->isValid())
    return;

  const SkBitmap& bitmap = surface->bitmap();
  if (bitmap.empty())
    return;

  // Create a CGImage from the Skia bitmap
  CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
  CGBitmapInfo bitmapInfo = kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst;

  CGDataProviderRef provider = CGDataProviderCreateWithData(
    nullptr,
    bitmap.getPixels(),
    bitmap.computeByteSize(),
    nullptr);

  if (!provider) {
    CGColorSpaceRelease(cgColorSpace);
    return;
  }

  CGImageRef img = CGImageCreate(
    bitmap.width(),
    bitmap.height(),
    8,                          // bits per component
    32,                         // bits per pixel
    bitmap.rowBytes(),
    cgColorSpace,
    bitmapInfo,
    provider,
    nullptr,                    // decode
    false,                      // interpolate
    kCGRenderingIntentDefault);

  CGDataProviderRelease(provider);
  CGColorSpaceRelease(cgColorSpace);

  if (!img)
    return;

  // Get the AsepriteView and update its layer
  AsepriteView* view = asepriteView();
  if (view) {
    [view updateWithCGImage:img];
  }

  CGImageRelease(img);
}

void SkiaWindowIOS::setTextInput(bool state, const gfx::Point& screenCaretPos)
{
  AsepriteView* view = asepriteView();
  if (!view)
    return;

  dispatch_async(dispatch_get_main_queue(), ^{
    view.textInputEnabled = state;
    if (state) {
      [view becomeFirstResponder];
    }
    else {
      [view resignFirstResponder];
    }
  });
}

void SkiaWindowIOS::onClose()
{
  m_closing = true;
}

void SkiaWindowIOS::onResize(const gfx::Size& size)
{
  resizeSkiaSurface(size);
}

void SkiaWindowIOS::onDrawRect(const gfx::Rect& rect)
{
  // Drawing is handled by invalidateRegion
}

} // namespace os
