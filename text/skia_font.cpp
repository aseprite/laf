// LAF Text Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "text/skia_font.h"

#include "os/skia/skia_helpers.h"
#include "text/font_metrics.h"
#include "text/skia_font_mgr.h"

#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"

namespace text {

SkiaFont::SkiaFont(const SkFont& skFont)
  : m_skFont(skFont)
{
}

SkiaFont::~SkiaFont()
{
}

bool SkiaFont::isValid() const
{
  return true;
}

FontType SkiaFont::type()
{
  return FontType::Native;
}

TypefaceRef SkiaFont::typeface() const
{
  return base::make_ref<SkiaTypeface>(
    sk_ref_sp(m_skFont.getTypeface()));
}

float SkiaFont::metrics(FontMetrics* metrics) const
{
  SkFontMetrics skMetrics;
  float lineSpacing = m_skFont.getMetrics(&skMetrics);
  if (metrics) {
    metrics->top = skMetrics.fTop;
    metrics->ascent = skMetrics.fAscent;
    metrics->descent = skMetrics.fDescent;
    metrics->bottom = skMetrics.fBottom;
    metrics->leading = skMetrics.fLeading;
    metrics->avgCharWidth = skMetrics.fAvgCharWidth;
    metrics->maxCharWidth = skMetrics.fMaxCharWidth;
    metrics->xMin = skMetrics.fXMin;
    metrics->xMax = skMetrics.fXMax;
    metrics->xHeight = skMetrics.fXHeight;
    metrics->capHeight = skMetrics.fCapHeight;
    metrics->underlineThickness = skMetrics.fUnderlineThickness;
    metrics->underlinePosition = skMetrics.fUnderlinePosition;
    metrics->strikeoutThickness = skMetrics.fStrikeoutThickness;
    metrics->strikeoutPosition = skMetrics.fStrikeoutPosition;
  }
  return lineSpacing;
}

int SkiaFont::height() const
{
  return m_skFont.getMetrics(nullptr);
}

int SkiaFont::textLength(const std::string& str) const
{
  return std::ceil(
    m_skFont.measureText(
      str.c_str(),
      str.size(),
      SkTextEncoding::kUTF8,
      nullptr)
  );
}

float SkiaFont::measureText(const std::string& str,
                            gfx::RectF* outBounds,
                            const os::Paint* paint) const
{
  SkRect bounds;
  float width = m_skFont.measureText(
    str.c_str(),
    str.size(),
    SkTextEncoding::kUTF8,
    &bounds,
    paint ? &paint->skPaint(): nullptr);
  if (outBounds)
    *outBounds = os::from_skia(bounds);
  return width;
}

bool SkiaFont::isScalable() const
{
  return true;
}

void SkiaFont::setSize(int size)
{
  m_skFont.setSize(size);
}

bool SkiaFont::antialias() const
{
  return (m_skFont.getEdging() != SkFont::Edging::kAlias);
}

void SkiaFont::setAntialias(bool antialias)
{
  m_skFont.setEdging(antialias ? SkFont::Edging::kAntiAlias:
                                 SkFont::Edging::kAlias);
}

glyph_t SkiaFont::codePointToGlyph(codepoint_t codepoint) const
{
  return m_skFont.unicharToGlyph(codepoint);
}

gfx::RectF SkiaFont::getGlyphBounds(glyph_t glyph) const
{
  float widths;
  SkRect bounds;
  m_skFont.getWidthsBounds(&glyph, 1, &widths, &bounds, nullptr);
  return os::from_skia(bounds);
}

} // namespace text
