// LAF OS Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

//#define DEBUG_UPDATE_RECTS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/skia/skia_window_osx.h"

#include "base/log.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/osx/color_space.h"
#include "os/osx/event_queue.h"
#include "os/osx/view.h"
#include "os/osx/window.h"
#include "os/skia/resize_surface.h"
#include "os/skia/skia_color_space.h"
#include "os/skia/skia_display.h"
#include "os/skia/skia_surface.h"
#include "os/system.h"

#include "mac/SkCGUtils.h"

#if SK_SUPPORT_GPU

  #include "GrBackendSurface.h"
  #include "GrContext.h"
  #include "gl/GrGLDefines.h"
  #include "gl/GrGLInterface.h"
  #include "os/skia/skia_surface.h"
  #include <OpenGL/gl.h>

#endif

#include <algorithm>
#include <iostream>

namespace os {

class SkiaWindow::Impl : public OSXWindowImpl {
public:
  Impl(EventQueue* queue, SkiaDisplay* display,
       int width, int height, int scale)
    : m_display(display)
    , m_backend(Backend::NONE)
#if SK_SUPPORT_GPU
    , m_nsGL(nil)
    , m_nsPixelFormat(nil)
    , m_skSurface(nullptr)
#endif
  {
    m_closing = false;
    m_window = [[OSXWindow alloc] initWithImpl:this
                                         width:width
                                        height:height
                                         scale:scale];
  }

  ~Impl() {
#if SK_SUPPORT_GPU
    if (m_backend == Backend::GL)
      detachGL();
#endif
  }

  gfx::Size clientSize() const {
    return [m_window clientSize];
  }

  gfx::Size restoredSize() const {
    return [m_window restoredSize];
  }

  os::ColorSpacePtr colorSpace() const {
    ASSERT(m_window);
    if (auto defaultCS = os::instance()->displaysColorSpace())
      return defaultCS;

    return convert_nscolorspace_to_os_colorspace([m_window colorSpace]);
  }

  int scale() const {
    return [m_window scale];
  }

  void setScale(int scale) {
    [m_window setScale:scale];
  }

  float displayScale() const {
    return m_window.backingScaleFactor;
  }

  void setVisible(bool visible) {
    if (visible) {
      // Make the first OSXWindow as the main one.
      [m_window makeKeyAndOrderFront:nil];

      // The main window can be changed only when the NSWindow
      // is visible (i.e. when NSWindow::canBecomeMainWindow
      // returns YES).
      [m_window makeMainWindow];
    }
    else {
      [m_window close];
    }
  }

  void setTitle(const std::string& title) {
    [m_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
  }

  void setMousePosition(const gfx::Point& position) {
    [m_window setMousePosition:position];
  }

  bool setNativeMouseCursor(NativeCursor cursor) {
    return ([m_window setNativeMouseCursor:cursor] ? true: false);
  }

  bool setNativeMouseCursor(const os::Surface* surface,
                            const gfx::Point& focus,
                            const int scale) {
    return ([m_window setNativeMouseCursor:surface
                                     focus:focus
                                     scale:scale] ? true: false);
  }

  void invalidateRegion(const gfx::Region& rgn) {
    switch (m_backend) {

      case Backend::NONE:
        @autoreleasepool {
          gfx::Rect bounds = rgn.bounds(); // TODO use only the region?
          int scale = this->scale();
          float displayScale = this->displayScale();
          NSView* view = m_window.contentView;
          [view setNeedsDisplayInRect:
                  NSMakeRect(bounds.x*scale/displayScale,
                             view.frame.size.height - (bounds.y+bounds.h)*scale/displayScale,
                             bounds.w*scale/displayScale,
                             bounds.h*scale/displayScale)];

#if 0     // Do not refresh immediately. Note: This might be required
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
      case Backend::GL:
        if (m_skSurfaceDirect) {
          if (m_skSurface != m_skSurfaceDirect) {
            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrc);
            sk_sp<SkImage> snapshot = m_skSurface->makeImageSnapshot();

            m_skSurfaceDirect->getCanvas()->drawImageRect(
              snapshot,
              SkRect::Make(SkIRect::MakeXYWH(0, 0, snapshot->width(), snapshot->height())),
              SkRect::Make(SkIRect::MakeXYWH(0, 0, m_skSurfaceDirect->width(), m_skSurfaceDirect->height())),
              &paint, SkCanvas::kStrict_SrcRectConstraint);
          }
          // We have use SkSurface::flush() explicitly because we are
          // going to call a native OpenGL function. Skia calls
          // flush() automatically only when you interact with their
          // API, but when you need to call the native API manually,
          // it's necessary to call it explicitly.
          m_skSurfaceDirect->flush();
        }
        if (m_nsGL) {
          // Flush all commands and swap front/back buffers (this will
          // make the back buffer visible to the user)
          [m_nsGL flushBuffer];

          // Copy the front buffer to the back buffer. This is because
          // in the non-GPU backend the back buffer persists between
          // each flip: each frame is not drawn from scratch, so we
          // can draw only the differences. In this way we keep both
          // buffers in sync to support this kind of behavior.
          //
          // TODO make this configurable in case that we want to draw
          //      each frame from scratch, e.g. game-like frame rendering
          if (m_skSurfaceDirect) {
            const int w = m_skSurfaceDirect->width();
            const int h = m_skSurfaceDirect->height();
            glReadBuffer(GL_FRONT);
            glDrawBuffer(GL_BACK);
            glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
          }
        }
        break;
#endif

    }
  }

  void setTranslateDeadKeys(bool state) {
    OSXView* view = (OSXView*)m_window.contentView;
    [view setTranslateDeadKeys:(state ? YES: NO)];
  }

  void* handle() {
    return (__bridge void*)m_window;
  }

  // OSXWindowImpl impl

  void onQueueEvent(Event& ev) override {
    ev.setDisplay(m_display);
    os::queue_event(ev);
  }

  void onClose() override {
    m_closing = true;
  }

  void onResize(const gfx::Size& size) override {
    bool gpu = os::instance()->gpuAcceleration();
    (void)gpu;

#if SK_SUPPORT_GPU
    if (gpu && attachGL()) {
      m_backend = Backend::GL;
    }
    else
#endif
    {
#if SK_SUPPORT_GPU
      detachGL();
#endif
      m_backend = Backend::NONE;
    }

#if SK_SUPPORT_GPU
    if (m_nsGL && m_display->isInitialized())
      createRenderTarget(size);
#endif

    m_display->resize(size);
    if (m_resizeSurface) {
      m_resizeSurface.draw(m_display);
      if (m_backend == Backend::GL)
        invalidateRegion(gfx::Region(gfx::Rect(size)));
    }
  }

  void onDrawRect(const gfx::Rect& rect) override {
    switch (m_backend) {

      case Backend::NONE:
        paintGC(rect);
        break;

#if SK_SUPPORT_GPU
      case Backend::GL:
        // Do nothing
        break;
#endif
    }
  }

  void onWindowChanged() override {
#if SK_SUPPORT_GPU
    if (m_nsGL)
      [m_nsGL setView:[m_window contentView]];
#endif
  }

  void onStartResizing() override {
    ASSERT(!m_resizeSurface);
    m_resizeSurface.create(m_display);
  }

  void onEndResizing() override {
    ASSERT(m_resizeSurface);
    m_resizeSurface.reset();

    // Generate the resizing display event for the user.
    Event ev;
    ev.setType(Event::ResizeDisplay);
    ev.setDisplay(m_display);
    os::queue_event(ev);
  }

  void onChangeBackingProperties() override {
    if (m_window && m_display)
      m_display->setColorSpace(colorSpace());
  }

private:
#if SK_SUPPORT_GPU
  bool attachGL() {
    if (m_nsGL)
      return true;
    try {
      // set up pixel format
      std::vector<NSOpenGLPixelFormatAttribute> attr;
      attr.push_back(NSOpenGLPFAAccelerated);
      attr.push_back(NSOpenGLPFAClosestPolicy);
      attr.push_back(NSOpenGLPFADoubleBuffer);
      attr.push_back(NSOpenGLPFAOpenGLProfile);
      attr.push_back(NSOpenGLProfileVersion3_2Core);
      attr.push_back(NSOpenGLPFAColorSize);
      attr.push_back(24);
      attr.push_back(NSOpenGLPFAAlphaSize);
      attr.push_back(8);
      attr.push_back(NSOpenGLPFADepthSize);
      attr.push_back(0);
      attr.push_back(NSOpenGLPFAStencilSize);
      attr.push_back(8);
      attr.push_back(NSOpenGLPFASampleBuffers);
      attr.push_back(0);
      attr.push_back(0);

      m_nsPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:&attr[0]];
      if (nil == m_nsPixelFormat)
        return false;

      m_nsGL = [[NSOpenGLContext alloc] initWithFormat:m_nsPixelFormat
                                          shareContext:nil];
      if (!m_nsGL) {
        m_nsPixelFormat = nil;
        return false;
      }

      [m_nsGL setView:m_window.contentView];

      GLint swapInterval = 0;   // disable vsync
      [m_nsGL setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

      // TODO publish an API method to change this value to YES in
      // case that the user wants full control of the Retina pixels.
      [m_window.contentView setWantsBestResolutionOpenGLSurface:NO];

      [m_nsGL makeCurrentContext];

      m_glInterfaces.reset(GrGLCreateNativeInterface());
      if (!m_glInterfaces || !m_glInterfaces->validate()) {
        LOG(ERROR) << "OS: Cannot create GL interfaces\n";
        detachGL();
        return false;
      }

      m_grCtx = GrContext::MakeGL(m_glInterfaces);
      if (!m_grCtx) {
        LOG(ERROR) << "OS: Cannot create GrContext\n";
        detachGL();
        return false;
      }

      LOG("OS: Using OpenGL backend\n");

      createRenderTarget(clientSize());
    }
    catch (const std::exception& ex) {
      LOG(ERROR) << "OS: Cannot create GL context: " << ex.what() << "\n";
      detachGL();
      return false;
    }
    return true;
  }

  void detachGL() {
    if (m_nsGL) {
      LOG(INFO, "OS: detach GL context\n");
      m_nsGL = nil;
    }

    m_skSurface.reset(nullptr);
    m_skSurfaceDirect.reset(nullptr);
    m_grCtx.reset(nullptr);
  }

  void createRenderTarget(const gfx::Size& size) {
    [m_nsGL update];

    // Setup of a SkSurface connected to the framebuffer

    const int scale = this->scale();
    auto colorSpace = ((SkiaColorSpace*)this->colorSpace().get())->skColorSpace();

    GrGLint buffer;
    m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_FRAMEBUFFER_BINDING, &buffer);

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = buffer;
    fbInfo.fFormat = GR_GL_RGBA8;

    SkSurfaceProps props(SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
                         SkSurfaceProps::kLegacyFontHost_InitType);

    GLint stencilBits;
    [m_nsGL.pixelFormat getValues:&stencilBits
                     forAttribute:NSOpenGLPFAStencilSize
                 forVirtualScreen:0];

    GLint sampleCount;
    [m_nsGL.pixelFormat getValues:&sampleCount
                     forAttribute:NSOpenGLPFASamples
                 forVirtualScreen:0];
    sampleCount = std::max(sampleCount, 1);

    GrBackendRenderTarget backendRT(size.w, size.h,
                                    sampleCount,
                                    stencilBits,
                                    fbInfo);

    m_skSurface.reset(nullptr); // set m_skSurface comparing with the old m_skSurfaceDirect
    m_skSurfaceDirect =
      SkSurface::MakeFromBackendRenderTarget(
        m_grCtx.get(), backendRT,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        colorSpace,
        &props);

    if (scale == 1 && m_skSurfaceDirect) {
      LOG("OS: Using GL direct surface %p\n", m_skSurfaceDirect.get());
      m_skSurface = m_skSurfaceDirect;
    }
    else {
      LOG("OS: Using double buffering\n");

      SkImageInfo info = SkImageInfo::Make(
        std::max(1, size.w / scale),
        std::max(1, size.h / scale),
        kN32_SkColorType,
        kOpaque_SkAlphaType,
        colorSpace);

      m_skSurface =
        SkSurface::MakeRenderTarget(
          m_grCtx.get(), SkBudgeted::kNo,
          info, sampleCount, &props);
    }

    if (!m_skSurface)
      throw std::runtime_error("Error creating surface for main display");

    m_display->setSkiaSurface(new SkiaSurface(m_skSurface));
  }

#endif

  void paintGC(const gfx::Rect& rect) {
    if (!m_display->isInitialized())
      return;

    if (rect.isEmpty())
      return;

    NSRect viewBounds = m_window.contentView.bounds;
    int scale = this->scale();
    float displayScale = this->displayScale();

    SkiaSurface* surface = static_cast<SkiaSurface*>(m_display->getSurface());
    const SkBitmap& origBitmap = surface->bitmap();

    SkBitmap bitmap;
    if (scale == 1) {
      // Create a subset to draw on the view
      if (!origBitmap.extractSubset(
            &bitmap, SkIRect::MakeXYWH(rect.x,
                                       (viewBounds.size.height*displayScale-(rect.y+rect.h)),
                                       rect.w,
                                       rect.h)))
        return;
    }
    else {
      // Create a bitmap to draw the original one scaled. This is
      // faster than doing the scaling directly in
      // CGContextDrawImage(). This avoid a slow path where the
      // internal macOS argb32_image_mark_RGB32() function is called
      // (which is a performance hit).
      if (!bitmap.tryAllocN32Pixels(rect.w, rect.h, true))
        return;

      SkCanvas canvas(bitmap);
      canvas.drawBitmapRect(origBitmap,
                            SkIRect::MakeXYWH(rect.x/scale,
                                              (viewBounds.size.height*displayScale-(rect.y+rect.h))/scale,
                                              rect.w/scale,
                                              rect.h/scale),
                            SkRect::MakeXYWH(0, 0, rect.w, rect.h),
                            nullptr);
    }

    @autoreleasepool {
      NSGraphicsContext* gc = [NSGraphicsContext currentContext];
      CGContextRef cg = (CGContextRef)[gc graphicsPort];
      CGColorSpaceRef colorSpace = CGDisplayCopyColorSpace(CGMainDisplayID());
      CGImageRef img = SkCreateCGImageRefWithColorspace(bitmap, colorSpace);
      if (img) {
        CGRect r = CGRectMake(viewBounds.origin.x+rect.x/displayScale,
                              viewBounds.origin.y+rect.y/displayScale,
                              rect.w/displayScale, rect.h/displayScale);

        CGContextSaveGState(cg);
        CGContextSetInterpolationQuality(cg, kCGInterpolationNone);
        CGContextDrawImage(cg, r, img);
#ifdef DEBUG_UPDATE_RECTS
        {
          static int i = 0;
          i = (i+1) % 8;
          CGContextSetRGBStrokeColor(cg,
                                     (i & 1 ? 1.0f: 0.0f),
                                     (i & 2 ? 1.0f: 0.0f),
                                     (i & 4 ? 1.0f: 0.0f), 1.0f);
          CGContextStrokeRectWithWidth(cg, r, 2.0f);
        }
#endif
        CGContextRestoreGState(cg);
        CGImageRelease(img);
      }
      CGColorSpaceRelease(colorSpace);
    }
  }

  SkiaDisplay* m_display = nullptr;
  Backend m_backend;
  bool m_closing;
  OSXWindow* m_window = nullptr;
  ResizeSurface m_resizeSurface;     // Surface used for live resizing.

#if SK_SUPPORT_GPU
  sk_sp<const GrGLInterface> m_glInterfaces;
  NSOpenGLContext* m_nsGL;
  NSOpenGLPixelFormat* m_nsPixelFormat;
  sk_sp<GrContext> m_grCtx;
  sk_sp<SkSurface> m_skSurfaceDirect;
  sk_sp<SkSurface> m_skSurface;
#endif
};

SkiaWindow::SkiaWindow(EventQueue* queue, SkiaDisplay* display,
                       int width, int height, int scale)
  : m_impl(new Impl(queue, display,
                    width, height, scale))
{
}

SkiaWindow::~SkiaWindow()
{
  destroyImpl();
}

void SkiaWindow::destroyImpl()
{
  delete m_impl;
  m_impl = nullptr;
}

ColorSpacePtr SkiaWindow::colorSpace() const
{
  if (m_impl)
    return m_impl->colorSpace();
  else
    return nullptr;
}

int SkiaWindow::scale() const
{
  if (m_impl)
    return m_impl->scale();
  else
    return 1;
}

void SkiaWindow::setScale(int scale)
{
  if (m_impl)
    m_impl->setScale(scale);
}

void SkiaWindow::setVisible(bool visible)
{
  if (!m_impl)
    return;

  m_impl->setVisible(visible);
}

void SkiaWindow::maximize()
{
}

bool SkiaWindow::isMaximized() const
{
  return false;
}

bool SkiaWindow::isMinimized() const
{
  return false;
}

gfx::Size SkiaWindow::clientSize() const
{
  if (!m_impl)
    return gfx::Size(0, 0);

  return m_impl->clientSize();
}

gfx::Size SkiaWindow::restoredSize() const
{
  if (!m_impl)
    return gfx::Size(0, 0);

  return m_impl->restoredSize();
}

void SkiaWindow::setTitle(const std::string& title)
{
  if (!m_impl)
    return;

  m_impl->setTitle(title);
}

void SkiaWindow::captureMouse()
{
}

void SkiaWindow::releaseMouse()
{
}

void SkiaWindow::setMousePosition(const gfx::Point& position)
{
  if (m_impl)
    m_impl->setMousePosition(position);
}

bool SkiaWindow::setNativeMouseCursor(NativeCursor cursor)
{
  if (m_impl)
    return m_impl->setNativeMouseCursor(cursor);
  else
    return false;
}

bool SkiaWindow::setNativeMouseCursor(const Surface* surface,
                                      const gfx::Point& focus,
                                      const int scale)
{
  if (m_impl)
    return m_impl->setNativeMouseCursor(surface, focus, scale);
  else
    return false;
}

void SkiaWindow::invalidateRegion(const gfx::Region& rgn)
{
  if (m_impl)
    m_impl->invalidateRegion(rgn);
}

void SkiaWindow::setTranslateDeadKeys(bool state)
{
  if (m_impl)
    m_impl->setTranslateDeadKeys(state);
}

void* SkiaWindow::handle()
{
  if (m_impl)
    return (void*)m_impl->handle();
  else
    return nullptr;
}

} // namespace os
