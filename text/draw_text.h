// LAF Text Library
// Copyright (c) 2022-2024  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef LAF_TEXT_DRAW_TEXT_H_INCLUDED
#define LAF_TEXT_DRAW_TEXT_H_INCLUDED
#pragma once

#include "base/ref.h"
#include "base/string.h"
#include "gfx/color.h"
#include "gfx/fwd.h"
#include "gfx/point.h"
#include "text/font.h"
#include "text/font_mgr.h"
#include "text/fwd.h"
#include "text/shaper_features.h"

namespace os {
class Surface;
class Paint;
} // namespace os

namespace text {

enum class TextAlign { Left, Center, Right };

class DrawTextDelegate {
public:
  virtual ~DrawTextDelegate() {}

  // This is called before drawing the character.
  virtual void preProcessChar(const int index,
                              const codepoint_t codepoint,
                              gfx::Color& fg,
                              gfx::Color& bg,
                              const gfx::Rect& charBounds)
  {
    // Do nothing
  }

  virtual bool preDrawChar(const gfx::Rect& charBounds)
  {
    // Returns false if the process should stop here.
    return true;
  }

  virtual void postDrawChar(const gfx::Rect& charBounds)
  {
    // Do nothing
  }
};

// The surface can be nullptr just to process the string
// (e.g. measure how much space will use the text without drawing
// it). It uses FreeType2 library and harfbuzz. Doesn't support RTL
// (right-to-left) languages.
[[deprecated]]
gfx::Rect draw_text(os::Surface* surface,
                    const FontMgrRef& fontMgr,
                    const FontRef& font,
                    const std::string& text,
                    gfx::Color fg,
                    gfx::Color bg,
                    int x,
                    int y,
                    DrawTextDelegate* delegate = nullptr,
                    ShaperFeatures features = {});

void draw_text(os::Surface* surface,
               const TextBlobRef& blob,
               const gfx::PointF& pos,
               const os::Paint* paint = nullptr);

// Uses Skia's SkTextUtils::Draw() to draw text (doesn't depend on
// harfbuzz or big dependencies, useful to print English text only).
void draw_text(os::Surface* surface,
               const FontRef& font,
               const std::string& text,
               gfx::PointF pos,
               const os::Paint* paint = nullptr,
               const TextAlign textAlign = TextAlign::Left);

// Uses Skia's SkTextUtils::Draw() to draw text (doesn't depend on
// harfbuzz or big dependencies, useful to print English text only).
inline void draw_text(os::Surface* surface,
                      const FontRef& font,
                      const std::string& text,
                      const gfx::Point& pos,
                      const os::Paint* paint = nullptr,
                      const TextAlign textAlign = TextAlign::Left)
{
  draw_text(surface, font, text, gfx::PointF(pos), paint, textAlign);
}

// Uses SkShaper::Make() to draw text (harfbuzz if available),
// useful for RTL (right-to-left) languages. Avoid this function if
// you are not going to translate your app to non-English languages
// (prefer os::draw_text() when possible).
void draw_text_with_shaper(os::Surface* surface,
                           const FontMgrRef& fontMgr,
                           const FontRef& font,
                           const std::string& text,
                           gfx::PointF pos,
                           const os::Paint* paint = nullptr,
                           const TextAlign textAlign = TextAlign::Left);

} // namespace text

#endif
