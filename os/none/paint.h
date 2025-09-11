// LAF OS Library
// Copyright (c) 2019-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_NONE_PAINT_H_INCLUDED
#define OS_NONE_PAINT_H_INCLUDED
#pragma once

#include "gfx/color.h"

namespace os {

class ColorSpace;

// Dummy Paint implementation for non-GUI apps/laf (when LAF_SKIA is
// false)
class Paint : public PaintBase {
public:
  bool antialias() const { return m_antialias; }
  void antialias(const bool state) { m_antialias = state; }

  Style style() const { return m_style; }
  void style(const Style style) { m_style = style; }

  gfx::Color color() const { return m_color; }
  void color(const gfx::Color color, const os::ColorSpace* cs = nullptr) { m_color = color; }

  float strokeWidth() const { return m_strokeWidth; }
  void strokeWidth(const float strokeWidth) { m_strokeWidth = strokeWidth; }

  BlendMode blendMode() const { return m_blendMode; }
  void blendMode(const BlendMode blendMode) { m_blendMode = blendMode; }

private:
  bool m_antialias = false;
  Style m_style = Fill;
  // Opaque black must be the default (to match SkPaint default)
  gfx::Color m_color = gfx::rgba(0, 0, 0, 255);
  float m_strokeWidth = 1.0f;
  BlendMode m_blendMode = BlendMode::SrcOver;
};

} // namespace os

#endif
