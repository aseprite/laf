// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2012-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SURFACE_FORMAT_H_INCLUDED
#define OS_SURFACE_FORMAT_H_INCLUDED
#pragma once

#include <cstdint>

namespace os {

  enum class SurfaceFormat {
    kRgbaSurfaceFormat,
  };

  enum class PixelAlpha {
    kOpaque,
    kPremultiplied,
    kStraight,
  };

  struct SurfaceFormatData {
    SurfaceFormat format;
    uint32_t bitsPerPixel;
    uint32_t redShift;
    uint32_t greenShift;
    uint32_t blueShift;
    uint32_t alphaShift;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t alphaMask;
    PixelAlpha pixelAlpha;
  };

} // namespace os

#endif
