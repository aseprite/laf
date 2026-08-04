// LAF Library
// Copyright (c) 2019-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/utf8_decode.h"
#include "os/os.h"
#include "text/text.h"

#include <cstdio>

using namespace os;
using namespace text;

const char* kTitle = "CTL";
const char* kLines[] = {
  "English",
  "Español",
  "Русский",                     // Russian
  "汉语",                        // Simplified Chinese
  "日本語",                      // Japanese
  "한국어",                      // Korean
  "العَرَبِيَّة‎", // Arabic
  "👍❤️😂☺️😯😢😡"                 // Emojis
};
constexpr size_t N = sizeof(kLines) / sizeof(kLines[0]);

std::vector<TextBlobRef> textBlobs;

base::codepoint_t inside_glyph_bounds(TextBlob* textBlob,
                                      const std::string& utf8text,
                                      const gfx::Point& mousePos,
                                      gfx::RectF& glyphBounds)
{
  // Once a TextBlob is created, it does't contain the original
  // Unicode code points data, so we have to decode the string again
  // if we want to match the glyph <-> codepoint association.
  //
  // This is used only to show in the window title bar which code
  // point we have the mouse on.
  base::utf8_decode decode(utf8text);

  base::codepoint_t codepoint = 0;
  textBlob->visitRuns([&](TextBlob::RunInfo& info) {
    for (int i = 0; i < info.glyphCount; ++i) {
      base::codepoint_t cp = decode.next();
      while (cp >= 0xfe00 && cp <= 0xfe0f) { // Skip variant selectors
        cp = decode.next();
      }

      const gfx::RectF rc = info.getGlyphBounds(i);
      if (rc.contains(gfx::PointF(mousePos))) {
        glyphBounds = rc;
        codepoint = cp;
        break;
      }
    }
  });
  return codepoint;
}

void draw_window(Window* window,
                 const FontMgrRef& fontMgr,
                 const FontRef& font,
                 const gfx::Point& mousePos)
{
  Surface* surface = window->surface();
  const gfx::Rect rc = surface->bounds();

  Paint p;
  p.color(gfx::rgba(0, 0, 0));
  p.style(Paint::Fill);
  surface->drawRect(rc, p);

  p.color(gfx::rgba(255, 255, 255));

  // Create the text blobs just one time, and we cache them in textBlobs array
  if (textBlobs.empty()) {
    textBlobs.resize(N);
    for (size_t i = 0; i < N; ++i) {
      textBlobs[i] = TextBlob::MakeWithShaper(fontMgr, font, kLines[i]);
    }
  }

  gfx::RectF focusedGlyph;
  base::codepoint_t focusedCodepoint = 0;

  gfx::PointF pos(rc.w / 2, 0);
  for (size_t i = 0; i < N; ++i) {
    auto& blob = textBlobs[i];
    gfx::PointF textPos(pos.x - blob->bounds().w / 2, pos.y);

    draw_text(surface, blob, textPos, &p);

    // Check if the mouse is over one glyph of this text blob.
    if (!focusedCodepoint) {
      focusedCodepoint =
        inside_glyph_bounds(blob.get(), kLines[i], mousePos - textPos, focusedGlyph);
      if (focusedCodepoint)
        focusedGlyph.offset(textPos);
    }

    pos.y += font->lineHeight() + 4;
  }

  // Show Unicode code point of the hover char in the title bar.
  if (focusedCodepoint) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s - U+%04X", kTitle, focusedCodepoint);
    window->setTitle(buf);

    focusedGlyph.enlarge(1.0f);
    p.style(Paint::Style::Stroke);
    surface->drawRect(focusedGlyph, p);
  }
  else {
    window->setTitle(kTitle);
  }

  // Invalidates the whole window to show it on the screen.
  if (window->isVisible())
    window->invalidateRegion(gfx::Region(rc));
  else
    window->setVisible(true);
}

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(AppMode::GUI);

  FontMgrRef fontMgr = FontMgr::Make();
  FontRef font = fontMgr->defaultFont(32);
  if (!font) {
    std::printf("Font not found\n");
    return 1;
  }

  WindowRef window = system->makeWindow(800, 800, 2);
  window->setTitle(kTitle);

  system->finishLaunching();
  system->activateApp();

  // Wait until a key is pressed or the window is closed
  EventQueue* queue = system->eventQueue();
  gfx::Point mousePos;
  bool running = true;
  bool redraw = true;

  system->handleWindowResize = [&](Window* w) { draw_window(w, fontMgr, font, mousePos); };

  while (running) {
    // Pick next event in the queue (without waiting)
    Event ev;
    queue->getEvent(ev, 0.0);

    // If there are no more events in the queue (Event::None type),
    // redraw the window and wait for some event. We do this so we
    // don't need to redraw the text on each mouse movement (we can
    // process several mouse movements at the same time and redraw
    // when the messages stop).
    if (ev.type() == Event::None) {
      if (redraw) {
        redraw = false;
        draw_window(window.get(), fontMgr, font, mousePos);
      }
      queue->getEvent(ev);
    }

    switch (ev.type()) {
      case Event::CloseWindow: running = false; break;

      case Event::KeyDown:
        switch (ev.scancode()) {
          case kKeyEsc: running = false; break;
          case kKey1:
          case kKey2:
          case kKey3:
          case kKey4:
          case kKey5:
          case kKey6:
          case kKey7:
          case kKey8:
          case kKey9:
            // Set scale
            window->setScale(1 + (int)(ev.scancode() - kKey1));
            redraw = true;
            break;
          default:
            // Do nothing for other cases
            break;
        }
        break;

      case Event::ResizeWindow: redraw = true; break;

      case Event::MouseEnter:
      case Event::MouseMove:
        mousePos = ev.position();
        redraw = true;
        break;

      case Event::MouseLeave:
        mousePos = gfx::Point(-1, -1);
        redraw = true;
        break;

      default:
        // Do nothing
        break;
    }
  }

  return 0;
}
