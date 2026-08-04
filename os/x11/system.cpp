// LAF OS Library
// Copyright (C) 2021-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/x11/system.h"

#include "os/gl/gl_context_glx.h"
#include "os/x11/cursor.h"

#include <X11/Xcursor/Xcursor.h>
#include <X11/cursorfont.h>

#include <algorithm>
#include <array>

namespace os {

class XCursorImageX11 {
public:
  XCursorImageX11() : m_ximage(nullptr) {}
  ~XCursorImageX11() { reset(); }

  ::XcursorImage* recreate(int w, int h)
  {
    if (m_ximage && m_ximage->width == w && m_ximage->height == h)
      return m_ximage;
    reset();
    return m_ximage = XcursorImageCreate(w, h);
  }

  void reset()
  {
    if (m_ximage)
      XcursorImageDestroy(m_ximage);
    m_ximage = nullptr;
  }

private:
  ::XcursorImage* m_ximage;
};

// Cache of system cursors
std::array<CursorRef, int(NativeCursor::Cursors)> g_nativeCursors;
XCursorImageX11 g_cachedCursorImage;

SystemX11::~SystemX11()
{
  destroyInstance();
  g_nativeCursors.fill(nullptr);
  g_cachedCursorImage.reset();
}

CursorRef SystemX11::getNativeCursor(NativeCursor cursor)
{
  const int i = int(cursor);
  if (i < 0 || i >= int(NativeCursor::Cursors))
    return nullptr;
  if (g_nativeCursors[i].get())
    return g_nativeCursors[i];

  ::Display* display = X11::instance()->display();
  ::Cursor xcursor = X11_None;
  switch (cursor) {
    case NativeCursor::Hidden: {
      char data = 0;
      const Pixmap image =
        XCreateBitmapFromData(display, DefaultRootWindow(display), (char*)&data, 1, 1);

      XColor color;
      xcursor = XCreatePixmapCursor(display, image, image, &color, &color, 0, 0);

      XFreePixmap(display, image);
      break;
    }
    case NativeCursor::Arrow:     xcursor = XCreateFontCursor(display, XC_arrow); break;
    case NativeCursor::Crosshair: xcursor = XCreateFontCursor(display, XC_crosshair); break;
    case NativeCursor::IBeam:     xcursor = XCreateFontCursor(display, XC_xterm); break;
    case NativeCursor::Wait:      xcursor = XCreateFontCursor(display, XC_watch); break;
    case NativeCursor::Link:      xcursor = XCreateFontCursor(display, XC_hand1); break;
    case NativeCursor::Help:      xcursor = XCreateFontCursor(display, XC_question_arrow); break;
    case NativeCursor::Forbidden: xcursor = XCreateFontCursor(display, XC_X_cursor); break;
    case NativeCursor::Move:      xcursor = XCreateFontCursor(display, XC_fleur); break;
    case NativeCursor::SizeN:     xcursor = XCreateFontCursor(display, XC_top_side); break;
    case NativeCursor::SizeNS:    xcursor = XCreateFontCursor(display, XC_sb_v_double_arrow); break;
    case NativeCursor::SizeS:     xcursor = XCreateFontCursor(display, XC_bottom_side); break;
    case NativeCursor::SizeW:     xcursor = XCreateFontCursor(display, XC_left_side); break;
    case NativeCursor::SizeE:     xcursor = XCreateFontCursor(display, XC_right_side); break;
    case NativeCursor::SizeWE:    xcursor = XCreateFontCursor(display, XC_sb_h_double_arrow); break;
    case NativeCursor::SizeNW:    xcursor = XCreateFontCursor(display, XC_top_left_corner); break;
    case NativeCursor::SizeNE:    xcursor = XCreateFontCursor(display, XC_top_right_corner); break;
    case NativeCursor::SizeSW:    xcursor = XCreateFontCursor(display, XC_bottom_left_corner); break;
    case NativeCursor::SizeSE:    xcursor = XCreateFontCursor(display, XC_bottom_right_corner); break;
  }

  return g_nativeCursors[i] = make_ref<CursorX11>(xcursor);
}

CursorRef SystemX11::makeCursor(const Surface* surface, const gfx::Point& focus, const int scale)
{
  ASSERT(surface);
  ::Display* display = X11::instance()->display();

  // This X11 server doesn't support ARGB cursors.
  if (!XcursorSupportsARGB(display))
    return nullptr;

  SurfaceFormatData format;
  surface->getFormat(&format);

  // Only for 32bpp surfaces
  if (format.bitsPerPixel != 32)
    return nullptr;

  const int w = scale * surface->width();
  const int h = scale * surface->height();

  ::Cursor xcursor = X11_None;
  ::XcursorImage* image = g_cachedCursorImage.recreate(w, h);
  if (image != X11_None) {
    XcursorPixel* dst = image->pixels;
    for (int y = 0; y < h; ++y) {
      const uint32_t* src = (const uint32_t*)surface->getData(0, y / scale);
      for (int x = 0, u = 0; x < w; ++x, ++dst) {
        const uint32_t c = *src;
        *dst = (((c & format.alphaMask) >> format.alphaShift) << 24) |
               (((c & format.redMask) >> format.redShift) << 16) |
               (((c & format.greenMask) >> format.greenShift) << 8) |
               (((c & format.blueMask) >> format.blueShift));
        if (++u == scale) {
          u = 0;
          ++src;
        }
      }
    }

    // We have to limit the focus position inside the cursor area to
    // avoid crash from XcursorImageLoadCursor():
    //
    //   X Error of failed request:  BadMatch (invalid parameter attributes)
    //     Major opcode of failed request:  138 (RENDER)
    //     Minor opcode of failed request:  27 (RenderCreateCursor)
    image->xhot = std::clamp(scale * focus.x + scale / 2, 0, w - 1);
    image->yhot = std::clamp(scale * focus.y + scale / 2, 0, h - 1);
    xcursor = XcursorImageLoadCursor(display, image);
  }

  return make_ref<CursorX11>(xcursor);
}

std::unique_ptr<GpuContext> SystemX11::makePlatformGpuContext()
{
  return std::make_unique<GLContextGLX>(X11::instance()->display());
}

SystemRef System::makeX11()
{
  return make_ref<SystemX11>();
}

} // namespace os
