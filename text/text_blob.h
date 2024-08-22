// LAF Text Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef LAF_TEXT_TEXT_BLOB_H_INCLUDED
#define LAF_TEXT_TEXT_BLOB_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"
#include "text/fwd.h"
#include "text/text_blob.h"

#include <functional>
#include <string>

namespace text {

  // A TextBlob is a representation of certain Unicode text converted
  // into glyphs in a sequence of "runs." Each run indicates a range
  // of text that was converted to glyphs using one font (font + glyph
  // IDs + positions). One blob can use several runs to convert
  // different chunks of a Unicode text depending on which fonts (the
  // original one + fallback fonts) are used to create the final
  // representation of the text.
  //
  // The TextBlob doesn't store the original Unicode text. It stores a
  // sequence of glyphs. You can access those glyphs with
  // visitRuns(). The only way to match glyph IDs with UTF-8 ranges is
  // when the TextBlob is built and a RunHandler is specified.
  class TextBlob : public base::RefCount {
  public:
    struct Utf8Range {
      size_t begin = 0;
      size_t end = 0;
    };

    // Based on SkShaper::RunHandler::RunInfo and Buffer
    struct RunInfo {
      FontRef font;
      size_t glyphCount = 0;
      bool rtl = false;
      Utf8Range utf8Range;
      glyph_t* glyphs = nullptr;        // required
      gfx::PointF* positions = nullptr; // required, if (!offsets) put glyphs[i] at positions[i]
                                        // if (offsets) positions[i+1]-positions[i] are advances
      gfx::PointF* offsets = nullptr;   // optional, if ( offsets) put glyphs[i] at positions[i]+offsets[i]
      uint32_t* clusters = nullptr;     // optional, utf8+clusters[i] starts run which produced glyphs[i]
      gfx::PointF point;                // offset to add to all positions

      // It can be used only in RunHandler.
      Utf8Range getGlyphUtf8Range(size_t i) const;

      // It can be used in RunHandler and in RunVisitor.
      gfx::RectF getGlyphBounds(size_t i) const;
    };

    class RunHandler {
    public:
      virtual ~RunHandler() = default;
      virtual void commitRunBuffer(RunInfo& info) = 0;
    };

    explicit TextBlob(const gfx::RectF& bounds) : m_bounds(bounds) {}
    virtual ~TextBlob() = default;

    // Returns exact bounds that are required to draw this TextBlob.
    gfx::RectF bounds();

    // Visits each run in the TextBlob.
    using RunVisitor = std::function<void(RunInfo&)>;
    virtual void visitRuns(const RunVisitor& visitor) = 0;

    // Uses Skia's SkTextBlob::MakeFromText() to create the TextBlob,
    // it doesn't depend on HarfBuzz or big dependencies, useful to
    // print English-only text with just the given font as parameter.
    static TextBlobRef Make(
      const FontRef& font,
      const std::string& text);

    // Uses SkShaper::Make() to create the text blob (HarfBuzz if
    // available), useful for RTL (right-to-left) languages or when
    // fallback fonts are required (from the FontMgr) to get glyphs
    // from other fonts for other languages.
    //
    // Prefer this function if you offer i18n of your app. Avoid this
    // if you are not going to translate your app to non-English
    // languages. Prefer TextBlob::Make() when possible and if you
    // know that your font covers all possible Unicode chars with its
    // glyphs.
    static TextBlobRef MakeWithShaper(
      const FontMgrRef& fontMgr,
      const FontRef& font,
      const std::string& text,
      RunHandler* handler = nullptr);

  private:
    gfx::RectF m_bounds;
  };

} // namespace text

#endif
