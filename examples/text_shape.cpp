// LAF Library
// Copyright (c) 2024-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/string.h"
#include "base/time.h"
#include "os/os.h"
#include "text/text.h"

#include "os/skia/skia_helpers.h"
#include "os/skia/skia_surface.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPathEffect.h"
#include "include/effects/SkDashPathEffect.h"

#if LAF_WITH_CLIP
  #include "clip/clip.h"
#endif

#include <cstdio>

using namespace os;
using namespace text;

struct TextEdit {
  struct Box {
    TextBlob::Utf8Range utf8Range;
    gfx::RectF bounds;
  };

  base::tick_t caretTick;
  bool caretVisible = true;
  int caretIndex = 0;
  float caretHeight;
  float caretBaseLine;
  std::string text;
  TextBlobRef blob;
  std::vector<Box> boxes;

  class BoxBuilder : public TextBlob::RunHandler {
  public:
    BoxBuilder() {}
    const std::vector<Box>& boxes() const { return m_boxes; }

    // TextBlob::RunHandler impl
    void commitRunBuffer(TextBlob::RunInfo& info) override
    {
      Box box;
      for (int i = 0; i < info.glyphCount; ++i) {
        box.utf8Range = info.getGlyphUtf8Range(i);
        box.bounds = info.getGlyphBounds(i);
        m_boxes.push_back(box);
      }
    }

  private:
    std::vector<Box> m_boxes;
  };

  void makeCaretVisible()
  {
    caretTick = base::current_tick();
    caretVisible = true;
  }

  void makeBlob(FontMgrRef& fontMgr, FontRef& font)
  {
    // Create a blob without ligatures (to edit char by char)
    BoxBuilder handler;
    ShaperFeatures features;
    features.ligatures = false;
    blob = TextBlob::MakeWithShaper(fontMgr, font, text, &handler, features);
    boxes = handler.boxes();
  }
};

void draw_window(Window* window,
                 const FontMgrRef& fontMgr,
                 const FontRef& font,
                 const FontRef& fontBig,
                 const gfx::Point& mousePos,
                 TextEdit& edit)
{
  Surface* surface = window->surface();
  SurfaceLock lock(surface);
  const gfx::Rect rc = surface->bounds();

  Paint p;
  p.color(gfx::rgba(64, 64, 64));
  p.style(Paint::Fill);
  surface->drawRect(rc, p);

  gfx::PointF textPos;
  gfx::RectF box;

  if (edit.caretVisible) {
    int i = edit.caretIndex;
    float x, w, h;
    if (i < edit.boxes.size()) {
      x = edit.boxes[i].bounds.x;
      w = edit.boxes[i].bounds.w;
      h = std::max(edit.caretHeight, edit.boxes[i].bounds.h);
    }
    else {
      x = (!edit.boxes.empty() ? edit.boxes.back().bounds.x2() : 0);
      w = 4;
      h = edit.caretHeight;
    }

    box = gfx::RectF(textPos.x + x, textPos.y, w, h);

    surface->save();
    surface->clipRect(rc);
    p.color(gfx::rgba(240, 240, 240));
    surface->drawRect(box, p);
    p.color(gfx::rgba(64, 64, 64));
    if (edit.blob)
      draw_text(surface, edit.blob, textPos, &p);
    surface->restore();
  }

  surface->save();
  p.color(gfx::rgba(240, 240, 240));
  if (!box.isEmpty())
    static_cast<SkiaSurface*>(surface)->canvas().clipRect(to_skia(box), SkClipOp::kDifference);
  if (edit.blob)
    draw_text(surface, edit.blob, textPos, &p);
  surface->restore();

  // Draw current char
  surface->drawLine(0, rc.h / 2, rc.w, rc.h / 2, p);
  surface->drawLine(rc.w / 2, 0, rc.w / 2, rc.h, p);
  if (edit.caretIndex < edit.boxes.size()) {
    int i = edit.boxes[edit.caretIndex].utf8Range.begin;
    int j = edit.boxes[edit.caretIndex].utf8Range.end;
    gfx::PointF pos = gfx::PointF(rc.center());
    draw_text_with_shaper(surface, fontMgr, fontBig, edit.text.substr(i, j - i), pos, &p);

    FontMetrics metrics;
    fontBig->metrics(&metrics);

    pos.y -= metrics.ascent;
    const SkScalar intervals[] = { 4.0f, 4.0f };
    p.skPaint().setPathEffect(SkDashPathEffect::Make(intervals, 2, 0.0f));
    surface->drawLine(pos, gfx::PointF(rc.w, pos.y), p);
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
  FontRef font = fontMgr->defaultFont(48);
  FontRef fontBig = fontMgr->defaultFont(128);
  if (!font || !fontBig) {
    std::printf("Font not found\n");
    return 1;
  }

  WindowRef window = system->makeWindow(800, 600);
  window->setTitle("Text Shape");

  // Interpret dead keys + chars to compose unicode chars.  This is
  // useful for text editors (or when we focus a text editor in our
  // app, in this example we're always in the text editor).
  system->setTextInput(true);

  TextEdit edit;
  edit.text = "Hiragana ひらがな.";
  edit.makeBlob(fontMgr, font);
  edit.caretIndex = edit.boxes.size();
  edit.makeCaretVisible();

  FontMetrics metrics;
  font->metrics(&metrics);
  edit.caretHeight = metrics.descent - metrics.ascent + metrics.leading;
  edit.caretBaseLine = -metrics.ascent - metrics.leading;

  system->finishLaunching();
  system->activateApp();

  // Wait until a key is pressed or the window is closed
  EventQueue* queue = system->eventQueue();
  gfx::Point mousePos;
  bool running = true;
  bool redraw = true;
  while (running) {
    if (redraw) {
      redraw = false;
      draw_window(window.get(), fontMgr, font, fontBig, mousePos, edit);
    }

    Event ev;
    queue->getEvent(ev, 0.5);

    // Timer timeout (blink caret)
    while ((base::current_tick() - edit.caretTick) > 500) {
      edit.caretTick += 500;
      edit.caretVisible = !edit.caretVisible;
      redraw = true;
    }

    switch (ev.type()) {
      case Event::CloseWindow: running = false; break;

      case Event::KeyDown:
        switch (ev.scancode()) {
          case kKeyEsc: running = false; break;

          // Go to the beginning
          case kKeyHome:
          case kKeyUp:
            if (edit.caretIndex != 0) {
              edit.caretIndex = 0;
            }
            edit.makeCaretVisible();
            redraw = true;
            break;

          // Go to the end
          case kKeyEnd:
          case kKeyDown:
            if (edit.caretIndex != edit.boxes.size()) {
              edit.caretIndex = edit.boxes.size();
            }
            edit.makeCaretVisible();
            redraw = true;
            break;

          // Go to previous character/cluster
          case kKeyLeft:
            if (edit.caretIndex > 0) {
              --edit.caretIndex;
            }
            edit.makeCaretVisible();
            redraw = true;
            break;

          // Go to next character/cluster
          case kKeyRight:
            if (edit.caretIndex < edit.boxes.size()) {
              ++edit.caretIndex;
            }
            edit.makeCaretVisible();
            redraw = true;
            break;

          case kKeyBackspace:
            if (edit.caretIndex == 0)
              break;

            --edit.caretIndex;
            [[fallthrough]];

          case kKeyDel:
            if (edit.caretIndex < edit.boxes.size()) {
              auto utf8Range = edit.boxes[edit.caretIndex].utf8Range;
              edit.text.erase(utf8Range.begin, utf8Range.end - utf8Range.begin);
            }
            edit.makeBlob(fontMgr, font);
            edit.makeCaretVisible();
            redraw = true;
            break;

          default:
            edit.makeCaretVisible();
            redraw = true;

            if (!ev.isDeadKey()) {
              int chr = ev.unicodeChar();
              if (chr) {
                std::string newUtf8Str = ev.unicodeCharAsUtf8();

#if LAF_WITH_CLIP
                if ((ev.scancode() == kKeyV) && (ev.modifiers() & kKeyCtrlModifier) != 0) {
                  clip::get_text(newUtf8Str);
                }
#endif

                if (edit.caretIndex < edit.boxes.size()) {
                  size_t pos = edit.boxes[edit.caretIndex].utf8Range.begin;
                  edit.text.insert(pos, newUtf8Str);
                }
                else {
                  edit.text += newUtf8Str;
                }

                size_t oldSize = edit.boxes.size();
                edit.makeBlob(fontMgr, font);
                edit.caretIndex += edit.boxes.size() - oldSize;
              }
            }
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
