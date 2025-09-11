// LAF OS Library
// Copyright (c) 2022-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_PAINT_H_INCLUDED
#define OS_SKIA_PAINT_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "os/skia/skia_color_space.h"

#include "include/core/SkBlendMode.h"
#include "include/core/SkPaint.h"

namespace os {

// SkPaint wrapper, information how to paint a shape/primitive in a
// canvas (stroke, fill or both; stroke width; color, etc.).
class Paint : public PaintBase {
public:
  bool antialias() const { return m_skPaint.isAntiAlias(); }
  void antialias(const bool state) { m_skPaint.setAntiAlias(state); }

  Style style() const { return static_cast<Style>(m_skPaint.getStyle()); }
  void style(const Style style) { m_skPaint.setStyle(static_cast<SkPaint::Style>(style)); }

  gfx::Color color() const
  {
    const SkColor c = m_skPaint.getColor();
    return gfx::rgba(SkColorGetR(c), SkColorGetG(c), SkColorGetB(c), SkColorGetA(c));
  }

  void color(const gfx::Color c, const os::ColorSpace* cs = nullptr)
  {
    if (cs) {
      const auto* skiaCS = static_cast<const os::SkiaColorSpace*>(cs);
      m_skPaint.setColor4f(
        SkColor4f::FromColor(
          SkColorSetARGB(gfx::geta(c), gfx::getr(c), gfx::getg(c), gfx::getb(c))),
        skiaCS->skColorSpace().get());
    }
    else {
      m_skPaint.setColor(SkColorSetARGB(gfx::geta(c), gfx::getr(c), gfx::getg(c), gfx::getb(c)));
    }
  }

  float strokeWidth() const { return m_skPaint.getStrokeWidth(); }
  void strokeWidth(const float strokeWidth) { return m_skPaint.setStrokeWidth(strokeWidth); }

  BlendMode blendMode() const
  {
    auto bm = m_skPaint.asBlendMode();
    if (bm.has_value()) {
      return static_cast<BlendMode>(*bm);
    }
    else
      return BlendMode::Src;
  }
  void blendMode(const BlendMode blendMode)
  {
    m_skPaint.setBlendMode(static_cast<SkBlendMode>(blendMode));
  }

  const SkPaint& skPaint() const { return m_skPaint; }
  SkPaint& skPaint() { return m_skPaint; }

private:
  SkPaint m_skPaint;
};

static_assert((int)SkPaint::kFill_Style == (int)Paint::Fill &&
                (int)SkPaint::kStroke_Style == (int)Paint::Stroke &&
                (int)SkPaint::kStrokeAndFill_Style == (int)Paint::StrokeAndFill,
              "Paint styles don't match with Skia");

static_assert((int)SkBlendMode::kClear == (int)BlendMode::Clear &&
                (int)SkBlendMode::kSrc == (int)BlendMode::Src &&
                (int)SkBlendMode::kDst == (int)BlendMode::Dst &&
                (int)SkBlendMode::kSrcOver == (int)BlendMode::SrcOver &&
                (int)SkBlendMode::kDstOver == (int)BlendMode::DstOver &&
                (int)SkBlendMode::kSrcIn == (int)BlendMode::SrcIn &&
                (int)SkBlendMode::kDstIn == (int)BlendMode::DstIn &&
                (int)SkBlendMode::kSrcOut == (int)BlendMode::SrcOut &&
                (int)SkBlendMode::kDstOut == (int)BlendMode::DstOut &&
                (int)SkBlendMode::kSrcATop == (int)BlendMode::SrcATop &&
                (int)SkBlendMode::kDstATop == (int)BlendMode::DstATop &&
                (int)SkBlendMode::kXor == (int)BlendMode::Xor &&
                (int)SkBlendMode::kPlus == (int)BlendMode::Plus &&
                (int)SkBlendMode::kModulate == (int)BlendMode::Modulate &&
                (int)SkBlendMode::kScreen == (int)BlendMode::Screen &&
                (int)SkBlendMode::kLastCoeffMode == (int)BlendMode::LastCoeffMode &&
                (int)SkBlendMode::kOverlay == (int)BlendMode::Overlay &&
                (int)SkBlendMode::kDarken == (int)BlendMode::Darken &&
                (int)SkBlendMode::kLighten == (int)BlendMode::Lighten &&
                (int)SkBlendMode::kColorDodge == (int)BlendMode::ColorDodge &&
                (int)SkBlendMode::kColorBurn == (int)BlendMode::ColorBurn &&
                (int)SkBlendMode::kHardLight == (int)BlendMode::HardLight &&
                (int)SkBlendMode::kSoftLight == (int)BlendMode::SoftLight &&
                (int)SkBlendMode::kDifference == (int)BlendMode::Difference &&
                (int)SkBlendMode::kExclusion == (int)BlendMode::Exclusion &&
                (int)SkBlendMode::kMultiply == (int)BlendMode::Multiply &&
                (int)SkBlendMode::kLastSeparableMode == (int)BlendMode::LastSeparableMode &&
                (int)SkBlendMode::kHue == (int)BlendMode::Hue &&
                (int)SkBlendMode::kSaturation == (int)BlendMode::Saturation &&
                (int)SkBlendMode::kColor == (int)BlendMode::Color &&
                (int)SkBlendMode::kLuminosity == (int)BlendMode::Luminosity &&
                (int)SkBlendMode::kLastMode == (int)BlendMode::LastMode,
              "Blend modes don't match with Skia");

} // namespace os

#endif
