// LAF OS Library
// Copyright (c) 2020-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/osx/system.h"

#include "os/gl/gl_context_nsgl.h"
#include "os/osx/screen.h"

namespace os {

class CursorOSX : public os::Cursor {
public:
  CursorOSX(NSCursor* nsCursor) : m_nsCursor(nsCursor) {}
  ~CursorOSX() { m_nsCursor = nil; }

  CursorOSX(const CursorOSX&) = delete;
  CursorOSX& operator=(const CursorOSX&) = delete;

  void* nativeHandle() override { return (__bridge void*)m_nsCursor; }

private:
  NSCursor* m_nsCursor;
};

SystemOSX::~SystemOSX()
{
  destroyInstance();
}

CursorRef SystemOSX::makeCursor(const Surface* surface, const gfx::Point& focus, const int scale)
{
  ASSERT(surface);
  SurfaceFormatData format;
  surface->getFormat(&format);
  if (format.bitsPerPixel != 32)
    return nullptr;

  const int w = scale * surface->width();
  const int h = scale * surface->height();

  if (4 * w * h == 0)
    return nullptr;

  @autoreleasepool {
    NSBitmapImageRep* bmp = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:nil
                    pixelsWide:w
                    pixelsHigh:h
                 bitsPerSample:8
               samplesPerPixel:4
                      hasAlpha:YES
                      isPlanar:NO
                colorSpaceName:NSDeviceRGBColorSpace
                  bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                   bytesPerRow:w * 4
                  bitsPerPixel:32];
    if (!bmp)
      return nullptr;

    uint32_t* dst = (uint32_t*)[bmp bitmapData];
    for (int y = 0; y < h; ++y) {
      const uint32_t* src = (const uint32_t*)surface->getData(0, y / scale);
      for (int x = 0, u = 0; x < w; ++x, ++dst) {
        *dst = *src;
        if (++u == scale) {
          u = 0;
          ++src;
        }
      }
    }

    NSImage* img = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];
    if (!img)
      return nullptr;

    [img addRepresentation:bmp];

    NSCursor* nsCursor = [[NSCursor alloc]
      initWithImage:img
            hotSpot:NSMakePoint(scale * focus.x + scale / 2, scale * focus.y + scale / 2)];
    if (!nsCursor)
      return nullptr;

    return make_ref<CursorOSX>(nsCursor);
  }
}

gfx::Point SystemOSX::mousePosition() const
{
  NSScreen* primaryScreen = NSScreen.screens[0];
  NSPoint pos = [NSEvent mouseLocation];
  return gfx::Point(pos.x, primaryScreen.frame.size.height - pos.y);
}

void SystemOSX::setMousePosition(const gfx::Point& screenPosition)
{
  NSScreen* primaryScreen = NSScreen.screens[0];
  CGWarpMouseCursorPosition(
    CGPointMake(screenPosition.x, primaryScreen.frame.size.height - screenPosition.y));
}

gfx::Color SystemOSX::getColorFromScreen(const gfx::Point& screenPosition) const
{
  gfx::Color color = gfx::ColorNone;

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_15_0
  // TODO Use ScreenCaptureKit for macOS 15.0 as
  //      CGDisplayCreateImageForRect() function is obsoleted in
  //      this version.
#else
  CGImageRef image = CGDisplayCreateImageForRect(
    CGMainDisplayID(),
    CGRectMake(screenPosition.x, screenPosition.y, 1, 1));
  if (image) {
    CGBitmapInfo info = CGImageGetBitmapInfo(image);
    CGDataProviderRef provider = CGImageGetDataProvider(image);
    if (provider) {
      NSData* data = (__bridge NSData*)CGDataProviderCopyData(provider);
      const uint8_t* bytes = (const uint8_t*)data.bytes;

      // TODO support other formats
      const int bpp = CGImageGetBitsPerPixel(image);
      if (bpp == 32) {
        // TODO kCGBitmapByteOrder32Big
        if (info & kCGImageAlphaLast) {
          color = gfx::rgba(bytes[2], bytes[1], bytes[0], bytes[3]);
        }
        else {
          color = gfx::rgba(bytes[3], bytes[2], bytes[1], bytes[0]);
        }
      }

      // If we release the provider then CGImageRelease() crashes
      // CGDataProviderRelease(provider);
    }
    CGImageRelease(image);
  }
#endif
  return color;
}

ScreenRef SystemOSX::primaryScreen()
{
  return make_ref<ScreenOSX>(NSScreen.screens[0]);
}

void SystemOSX::listScreens(ScreenList& list)
{
  auto screens = [NSScreen screens];
  for (NSScreen* screen : screens)
    list.push_back(make_ref<ScreenOSX>(screen));
}

std::unique_ptr<GpuContext> SystemOSX::makePlatformGpuContext()
{
  return std::make_unique<GLContextNSGL>();
}

SystemRef System::makeOSX()
{
  return make_ref<SystemOSX>();
}

} // namespace os
