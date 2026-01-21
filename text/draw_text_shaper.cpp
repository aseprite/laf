// LAF Text Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "text/draw_text.h"

#include "os/paint.h"
#include "os/surface.h"
#include "text/sprite_sheet_font.h"
#include "text/text_blob.h"

#if LAF_SKIA
  #include "os/skia/skia_helpers.h"
  #include "os/skia/skia_surface.h"
  #include "text/skia_font.h"

  #include "include/core/SkCanvas.h"
#endif

namespace text {

namespace {

// Adapts the old DrawTextDelegate with new TextBlob run handlers.
class AdapterBuilder : public TextBlob::RunHandler {
public:
  AdapterBuilder(os::Surface* surface,
                 const std::string& text,
                 gfx::Color fg,
                 gfx::Color bg,
                 const gfx::PointF& origin,
                 DrawTextDelegate* delegate)
    : m_surface(surface)
    , m_text(text)
    , m_fg(fg)
    , m_bg(bg)
    , m_origin(origin)
    , m_delegate(delegate)
  {
  }

  // TextBlob::RunHandler impl
  void commitRunBuffer(TextBlob::RunInfo& info) override
  {
    if (info.clusters && info.glyphCount > 0) {
      float advanceX = 0.0f;

      os::Paint paint;
      paint.style(os::Paint::Fill);

      for (int i = 0; i < info.glyphCount; ++i) {
        int utf8Begin, utf8End;

        // LTR
        if (!info.rtl) {
          utf8Begin = info.utf8Range.begin + info.clusters[i];
          utf8End = (i + 1 < info.glyphCount ? info.utf8Range.begin + info.clusters[i + 1] :
                                               info.utf8Range.end);
        }
        // RTL
        else {
          utf8Begin = info.utf8Range.begin + info.clusters[i];
          utf8End = (i == 0 ? info.utf8Range.end : info.utf8Range.begin + info.clusters[i - 1]);
        }

        const std::string utf8text = m_text.substr(utf8Begin, utf8End - utf8Begin);

        gfx::RectF bounds = info.getGlyphBounds(i);
        bounds.offset(m_origin);

        advanceX += bounds.w;

        if (m_delegate) {
          const std::wstring widetext = base::from_utf8(utf8text);
          codepoint_t codepoint = 0;
          if (!widetext.empty()) {
            // On macOS and Linux wchar_t has 32-bits
            if constexpr (sizeof(wchar_t) >= 4) {
              codepoint = widetext[0];
            }
            // On Windows wchar_t has 16-bits (wide strings are UTF-16 strings)
            else if constexpr (sizeof(wchar_t) == 2) {
              codepoint = base::utf16_to_codepoint(widetext.size() > 1 ? widetext[1] : widetext[0],
                                                   widetext.size() > 1 ? widetext[0] : 0);
            }
            else {
              codepoint = 0;
            }
          }

          m_delegate->preProcessChar(utf8Begin, codepoint, m_fg, m_bg, bounds);
        }

        if (m_delegate)
          m_delegate->preDrawChar(bounds);

        if (m_surface && info.font) {
          if (info.font->type() == FontType::SpriteSheet) {
            const auto* spriteFont = static_cast<const SpriteSheetFont*>(info.font.get());
            const os::Surface* sheet = spriteFont->sheetSurface();
            const gfx::Rect sourceBounds = spriteFont->getGlyphBoundsOnSheet(info.glyphs[i]);

            m_surface->drawColoredRgbaSurface(
              sheet,
              m_fg,
              gfx::ColorNone,
              gfx::Clip(gfx::Point(info.positions[i] + m_origin + info.point), sourceBounds));
          }
#if LAF_SKIA
          else if (info.font->type() == FontType::Native) {
            SkGlyphID glyphs = info.glyphs[i];
            SkPoint positions = os::to_skia(info.positions[i]); //
            uint32_t clusters = info.clusters[i];
            paint.color(m_fg);
            static_cast<os::SkiaSurface*>(m_surface)->canvas().drawGlyphs(
              1,
              &glyphs,
              &positions,
              &clusters,
              utf8text.size(),
              utf8text.data(),
              os::to_skia(m_origin + info.point),
              static_cast<SkiaFont*>(info.font.get())->skFont(),
              paint.skPaint());
          }
#endif
        }

        if (m_delegate)
          m_delegate->postDrawChar(bounds);
      }
    }
  }

private:
  os::Surface* m_surface;
  const std::string& m_text;
  gfx::Color m_fg;
  gfx::Color m_bg;
  gfx::PointF m_origin;
  DrawTextDelegate* m_delegate;
};

} // anonymous namespace

gfx::Rect draw_text(os::Surface* surface,
                    const FontMgrRef& fontMgr,
                    const FontRef& font,
                    const std::string& text,
                    gfx::Color fg,
                    gfx::Color bg,
                    int x,
                    int y,
                    DrawTextDelegate* delegate,
                    ShaperFeatures features)
{
  TextBlobRef blob;
  if (delegate) {
    AdapterBuilder handler(surface, text, fg, bg, gfx::PointF(x, y), delegate);
    blob = TextBlob::MakeWithShaper(fontMgr, font, text, &handler, features);
  }
  else {
    blob = TextBlob::MakeWithShaper(fontMgr, font, text, nullptr, features);
    if (surface && blob) {
      // Paint background
      if (gfx::geta(bg) > 0) {
        os::Paint paint;
        paint.color(bg);
        paint.style(os::Paint::Fill);
        surface->drawRect(gfx::RectF(blob->bounds()).offset(x, y), paint);
      }

      os::Paint paint;
      paint.color(fg);
      draw_text(surface, blob, gfx::PointF(x, y), &paint);
    }
  }

  if (blob)
    return blob->bounds();
  return gfx::Rect();
}

void draw_text_with_shaper(os::Surface* surface,
                           const FontMgrRef& fontMgr,
                           const FontRef& font,
                           const std::string& text,
                           gfx::PointF pos,
                           const os::Paint* paint,
                           const TextAlign textAlign)
{
  if (!fontMgr || !font || font->type() != FontType::Native)
    return;

  const TextBlobRef blob = TextBlob::MakeWithShaper(fontMgr, font, text, nullptr);
  if (!blob)
    return;

  switch (textAlign) {
    case TextAlign::Left:   break;
    case TextAlign::Center: pos.x -= blob->bounds().w / 2.0f; break;
    case TextAlign::Right:  pos.x -= blob->bounds().w; break;
  }

  draw_text(surface, blob, pos, paint);
}

} // namespace text
