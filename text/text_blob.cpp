// LAF Text Library
// Copyright (c) 2024-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "text/text_blob.h"

#include "gfx/rect.h"
#include "gfx/size.h"
#include "text/font.h"
#include "text/font_metrics.h"
#include "text/sprite_text_blob.h"

#if LAF_SKIA
  #include "text/skia_text_blob.h"
#endif

namespace text {

gfx::RectF TextBlob::bounds()
{
  if (m_bounds.isEmpty()) {
    m_bounds = gfx::RectF(0, 0, 1, 1);
    visitRuns([this](RunInfo& info) {
      for (int i = 0; i < info.glyphCount; ++i) {
        m_bounds |= info.getGlyphBounds(i);
        if (info.font)
          m_bounds |= gfx::RectF(0, 0, 1, info.font->metrics(nullptr));
      }
    });
  }
  return m_bounds;
}

TextBlob::Utf8Range TextBlob::RunInfo::getGlyphUtf8Range(size_t i) const
{
  Utf8Range subRange;

  ASSERT(clusters);
  ASSERT(i < glyphCount);
  if (i >= glyphCount || !clusters)
    return subRange;

  // LTR
  if (!rtl) {
    subRange.begin = utf8Range.begin + clusters[i];
    subRange.end = (i + 1 < glyphCount ? utf8Range.begin + clusters[i + 1] : utf8Range.end);
  }
  // RTL
  else {
    subRange.begin = utf8Range.begin + clusters[i];
    subRange.end = (i == 0 ? utf8Range.end : utf8Range.begin + clusters[i - 1]);
  }
  return subRange;
}

gfx::RectF TextBlob::RunInfo::getGlyphBounds(const size_t i) const
{
  ASSERT(i < glyphCount);
  if (i >= glyphCount)
    return gfx::RectF();

  gfx::RectF bounds = font->getGlyphBounds(glyphs[i]);

  // Get bounds of whitespace from a space glyph.
  if (bounds.isEmpty()) {
    FontMetrics metrics;
    font->metrics(&metrics);
    bounds.w = font->getGlyphAdvance(font->codePointToGlyph(' '));
    bounds.h = std::abs(metrics.capHeight);
  }

  if (bounds.isEmpty())
    return bounds;

  bounds.offset(positions[i]);
  if (offsets)
    bounds.offset(offsets[i]);

  // Add global "point" offset to the bounds.
  bounds.offset(point);
  return bounds;
}

TextBlobRef TextBlob::Make(const FontRef& font, const std::string& text)
{
  ASSERT(font);
  switch (font->type()) {
    case FontType::SpriteSheet: return SpriteTextBlob::Make(font, text);

    case FontType::FreeType:
      ASSERT(false); // TODO impl
      return nullptr;

#if LAF_SKIA
    case FontType::Native: return SkiaTextBlob::Make(font, text);
#endif

    default: return nullptr;
  }
}

} // namespace text
