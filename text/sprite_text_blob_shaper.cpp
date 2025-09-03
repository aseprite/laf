// LAF Text Library
// Copyright (c) 2024-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "text/sprite_text_blob.h"

#include "base/ref.h"
#include "base/utf8_decode.h"
#include "text/font.h"
#include "text/font_metrics.h"
#include "text/font_mgr.h"
#include "text/sprite_sheet_font.h"

namespace text {

namespace {

// Used for sub-TextBlobs created from SpriteTextBlob to handle runs
// of text with callback fonts.
//
// As SpriteTextBlob can create other TextBlobs with sub-strings
// (using other kind of fonts like SkiaFont), those TextBlob
// (SkiaTextBlob) only see the sub-string part for the run (they don't
// know the whole string). With this OffsetHandler we adapt that
// sub-string information to the global string, adjusting the UTF-8
// range and the output position.
class OffsetHandler : public TextBlob::RunHandler {
public:
  OffsetHandler(RunHandler* original, const int offsetUtf8, const gfx::PointF& offsetOrigin)
    : m_original(original)
    , m_offsetUtf8(offsetUtf8)
    , m_offsetOrigin(offsetOrigin)
  {
  }

  // TextBlob::RunHandler impl
  void commitRunBuffer(TextBlob::RunInfo& info) override
  {
    // Adjust UTF8 range and position.
    info.utf8Range.begin += m_offsetUtf8;
    info.utf8Range.end += m_offsetUtf8;
    info.point = m_offsetOrigin;

    // Call the original RunHandler with the global info.
    if (m_original)
      m_original->commitRunBuffer(info);
  }

private:
  RunHandler* m_original;
  int m_offsetUtf8;
  gfx::PointF m_offsetOrigin;
};

} // anonymous namespace

TextBlobRef SpriteTextBlob::MakeWithShaper(const FontMgrRef& fontMgr,
                                           const FontRef& font,
                                           const std::string& text,
                                           TextBlob::RunHandler* handler)
{
  ASSERT(font);
  ASSERT(font->type() == FontType::SpriteSheet);
  ASSERT(dynamic_cast<SpriteSheetFont*>(font.get()));

  const auto* spriteFont = static_cast<const SpriteSheetFont*>(font.get());

  // TODO add configuration of the default falback font
  auto getFallbackFont = [](const FontMgrRef& fontMgr, const FontRef& font) -> FontRef {
    FontRef fallbackFont = font->fallback();
    if (!fallbackFont) {
      fallbackFont = fontMgr->defaultFont();
      fallbackFont->setSize(font->size());
      fallbackFont->setAntialias(font->antialias());
    }
    return fallbackFont;
  };

  // First iteration through the text/fonts/fallback fonts to
  // calculate the total text height and baseline to be used.
  FontMetrics metrics;
  font->metrics(&metrics);
  float baseline = -metrics.ascent;
  float textHeight = metrics.descent - metrics.ascent;
  {
    // TODO this utf8 iteration between fonts/fallback is duplicated
    //      here and below, we could try to merge this code
    base::utf8_decode decode(text);
    while (true) {
      const codepoint_t chr = decode.next();
      if (chr == 0)
        break;

      // Do not process newlines/control characters
      if (chr >= 10 && chr <= 20)
        continue;

      const glyph_t glyph = spriteFont->codePointToGlyph(chr);
      // Code point not found, use the fallback font or the FontMgr and
      // create a run using another TextBlob.
      if (glyph == 0) {
        base::utf8_decode subDecode = decode;
        while (true) {
          const base::utf8_decode prevSubDecode = subDecode;
          const codepoint_t subChr = subDecode.next();
          if (subChr == 0) {
            decode = subDecode;
            break;
          }

          // Continue the run until we find a glyph that can be
          // represent with the original font.
          if (spriteFont->codePointToGlyph(subChr) != 0) {
            decode = prevSubDecode; // Go back to the previous decode point
            break;
          }
        }

        // Calculate the max baseline/textHeight
        FontRef fallbackFont = getFallbackFont(fontMgr, font);
        FontMetrics fallbackMetrics;
        fallbackFont->metrics(&fallbackMetrics);
        baseline = std::max(baseline, -fallbackMetrics.ascent);
        textHeight = std::max(textHeight, fallbackMetrics.descent - fallbackMetrics.ascent);
      }
    }
  }

  Runs runs;
  Run run;
  auto addRun = [&runs, &run, &font, handler]() {
    if (handler && !run.subBlob) {
      TextBlob::RunInfo info;

      info.font = font;
      info.utf8Range = run.utf8Range;
      info.glyphCount = run.glyphs.size();
      info.glyphs = run.glyphs.data();
      info.positions = run.positions.data();
      info.clusters = run.clusters.data();

      handler->commitRunBuffer(info);
    }
    runs.push_back(run);
    run.clear();
  };

  gfx::RectF textBounds;
  gfx::PointF pos(0.0f, 0.0f);
  base::utf8_decode decode(text);
  while (true) {
    const int i = decode.pos() - text.begin();
    const codepoint_t chr = decode.next();
    run.utf8Range.end = i;
    if (chr == 0)
      break;

    if (chr >= 10 && chr <= 20)
      continue;

    const glyph_t glyph = spriteFont->codePointToGlyph(chr);
    // Code point not found, use the fallback font or the FontMgr and
    // create a run using another TextBlob.
    if (glyph == 0) {
      // Add run with original glyph
      if (!run.empty())
        addRun();

      base::utf8_decode subDecode = decode;
      while (true) {
        const base::utf8_decode prevSubDecode = subDecode;
        const codepoint_t subChr = subDecode.next();
        if (subChr == 0) {
          decode = subDecode;
          break;
        }

        // Continue the run until we find a glyph that can be
        // represent with the original font.
        if (spriteFont->codePointToGlyph(subChr) != 0) {
          decode = prevSubDecode; // Go back to the previous decode point
          break;
        }
      }

      const int j = decode.pos() - text.begin();

      // Add a run with "native" TextBlob (i.e. SkiaTextBlob).
      run.utf8Range.begin = i;
      run.utf8Range.end = j;

      // Align position between both fonts (font and fallbackFont)
      // in the baseline pos of the original font.
      FontRef fallbackFont = getFallbackFont(fontMgr, font);
      FontMetrics fallbackMetrics;
      fallbackFont->metrics(&fallbackMetrics);

      gfx::PointF alignedPos;
      alignedPos.x = pos.x;
      alignedPos.y = pos.y + baseline + fallbackMetrics.ascent;

      OffsetHandler subHandler(handler, i, alignedPos);
      run.subBlob = TextBlob::MakeWithShaper(fontMgr,
                                             fallbackFont,
                                             text.substr(i, j - i), // TODO use std::string_view
                                             &subHandler);
      if (run.subBlob) {
        run.positions.push_back(pos);

        textBounds |= run.subBlob->bounds();
        pos.x += run.subBlob->bounds().w;

        addRun();
      }

      // Restore beginning of UTF8 range for the next run
      run.utf8Range.begin = j;
      continue;
    }

    gfx::Rect glyphBounds = spriteFont->getGlyphBounds(glyph);
    if (glyphBounds.isEmpty())
      continue;

    gfx::PointF alignedPos;
    alignedPos.x = pos.x;
    alignedPos.y = pos.y + baseline + metrics.ascent;
    run.add(glyph, alignedPos, i - run.utf8Range.begin);

    glyphBounds.offset(pos);
    textBounds |= glyphBounds;
    pos.x += glyphBounds.w;
  }

  // Add last run
  if (!run.empty())
    addRun();

  auto blob = base::make_ref<SpriteTextBlob>(textBounds, font, std::move(runs));
  blob->setBaseline(baseline);
  blob->setTextHeight(textHeight);
  return blob;
}

} // namespace text
