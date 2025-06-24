// LAF Library
// Copyright (c) 2019-2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "os/os.h"
#include "text/text.h"

#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <string>
#include <vector>

using namespace os;
using namespace text;

class LogWindow {
public:
  LogWindow(const SystemRef& system)
    : m_system(system)
    , m_window(system->makeWindow(800, 600))
    , m_fontMgr(FontMgr::Make())
    , m_font(m_fontMgr->defaultFont(18))
    , m_lineHeight(m_font->metrics(nullptr))
  {
    m_system->setTextInput(m_textInput);
    m_window->setTitle(titleBar());

    recalcMaxLines();

    logLine("-- Events Log --");
  }

  bool processEvent(const Event& ev)
  {
    switch (ev.type()) {
      case Event::CloseApp:
      case Event::CloseWindow: return false;

      case Event::ResizeWindow:
        logLine("ResizeWindow size=%d,%d", m_window->width(), m_window->height());
        recalcMaxLines();
        break;

      case Event::DropFiles:
        logLine("DropFiles pos=%d,%d files={", ev.position().x, ev.position().y);
        for (const auto& file : ev.files()) {
          logLine("  \"%s\"", file.c_str());
        }
        logLine("}");
        break;

      case Event::MouseEnter:       logMouseEvent(ev, "MouseEnter"); break;
      case Event::MouseLeave:       logMouseEvent(ev, "MouseLeave"); break;
      case Event::MouseMove:        logMouseEvent(ev, "MouseMove"); break;
      case Event::MouseDown:        logMouseEvent(ev, "MouseDown"); break;
      case Event::MouseUp:          logMouseEvent(ev, "MouseUp"); break;
      case Event::MouseDoubleClick: logMouseEvent(ev, "MouseDoubleClick"); break;

      case Event::MouseWheel:
        m_mousePos = ev.position();
        logLine("MouseWheel pos=%d,%d %s=%d,%d%s",
                ev.position().x,
                ev.position().y,
                ev.preciseWheel() ? " preciseWheel" : "wheel",
                ev.wheelDelta().x,
                ev.wheelDelta().y,
                modifiersToString(ev.modifiers()).c_str());
        m_hue += double(ev.wheelDelta().x + ev.wheelDelta().y);
        break;

      case Event::KeyDown:
        if (ev.scancode() == kKeyEsc) {
          if (m_nextEscCloses)
            return false;
          m_nextEscCloses = true;
          logLine("-- Next KeyDown with kKeyEsc will close the window --");
        }
        else if (ev.scancode() == kKeyD) {
          m_textInput = !m_textInput;
          m_system->setTextInput(m_textInput);
          m_window->setTitle(titleBar());
        }
        else {
          m_nextEscCloses = false;
        }
        [[fallthrough]];
      case Event::KeyUp: {
        logLine("%s repeat=%d scancode=%d unicode=0x%x (%s)%s%s",
                (ev.type() == Event::KeyDown ? "KeyDown" : "KeyUp"),
                ev.repeat(),
                ev.scancode(),
                ev.unicodeChar(),
                ev.unicodeCharAsUtf8().c_str(),
                modifiersToString(ev.modifiers()).c_str(),
                (ev.isDeadKey() ? " DEADKEY" : ""));
        break;
      }

      case Event::TouchMagnify:
        logLine("TouchMagnify %.4g", ev.magnification());
        m_brushSize += 32 * ev.magnification();
        m_brushSize = std::clamp(m_brushSize, 1.0, 500.0);
        break;

      default:
        // Do nothing
        break;
    }
    return true;
  }

  void flush()
  {
    if (m_oldLogSize != m_textLog.size()) {
      int newlines = m_textLog.size() - m_oldLogSize;
      while (m_textLog.size() > m_maxlines)
        m_textLog.erase(m_textLog.begin());

      scrollAndDrawLog(newlines);

      m_oldLogSize = m_textLog.size();
    }
  }

private:
  std::string titleBar()
  {
    std::string title = "All Events";
    if (m_textInput)
      title += " w/Dead Keys";
    return title;
  }
  void recalcMaxLines() { m_maxlines = (m_window->height() - m_lineHeight) / m_lineHeight; }

  void scrollAndDrawLog(const int newlines)
  {
    Surface* surface = m_window->surface();
    SurfaceLock lock(surface);
    const gfx::Rect rc = surface->bounds();

    Paint p;
    p.style(Paint::Fill);
    p.color(gfx::rgba(0, 0, 0, 8));

    // Scroll old lines
    int i;
    if (m_textLog.size() >= m_maxlines) {
      int h = m_lineHeight * newlines;
      surface->scrollTo(rc, 0, -h);

      surface->drawRect(gfx::Rect(rc.x, rc.y, rc.w, rc.h - h), p);
      p.color(gfx::rgba(0, 0, 0));
      surface->drawRect(gfx::Rect(rc.x, rc.y + rc.h - h, rc.w, h), p);

      i = (m_textLog.size() - newlines);
    }
    // First lines without scroll
    else {
      i = m_oldLogSize;
      surface->drawRect(gfx::Rect(rc.x, rc.y, rc.w, i * m_lineHeight), p);
    }

    Paint paint;
    paint.color(gfx::rgba(255, 255, 255));
    for (; i < m_textLog.size(); ++i) {
      // Use shaper so we can paint emojis in the log
      draw_text(surface, m_textLog[i], gfx::PointF(0, i * m_lineHeight), &paint);
    }

    gfx::Rgb rgb(gfx::Hsv(m_hue, 1.0, 1.0));
    paint.color(gfx::rgba(rgb.red(), rgb.green(), rgb.blue()));
    paint.antialias(true);
    surface->drawCircle(m_mousePos.x, m_mousePos.y, m_brushSize, paint);

    // Invalidates the whole window to show it on the screen.
    if (m_window->isVisible())
      m_window->invalidateRegion(gfx::Region(rc));
    else
      m_window->setVisible(true);
  }

  void logMouseEvent(const Event& ev, const char* eventName)
  {
    const Event::MouseButton mb = ev.button();
    const PointerType pt = ev.pointerType();

    m_mousePos = ev.position();
    logLine("%s pos=%d,%d%s%s%s",
            eventName,
            ev.position().x,
            ev.position().y,
            (mb == Event::LeftButton   ? " LeftButton" :
             mb == Event::RightButton  ? " RightButton" :
             mb == Event::MiddleButton ? " MiddleButton" :
             mb == Event::X1Button     ? " X1Button" :
             mb == Event::X2Button     ? " X2Button" :
                                         ""),
            (pt == PointerType::Mouse    ? " Mouse" :
             pt == PointerType::Touchpad ? " Touchpad" :
             pt == PointerType::Touch    ? " Touch" :
             pt == PointerType::Pen      ? " Pen" :
             pt == PointerType::Cursor   ? " Cursor" :
             pt == PointerType::Eraser   ? " Eraser" :
                                           ""),
            modifiersToString(ev.modifiers()).c_str());
  }

  void logLine(const char* str, ...)
  {
    va_list ap;
    va_start(ap, str);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), str, ap);
    va_end(ap);

    TextBlobRef blob = TextBlob::MakeWithShaper(m_fontMgr, m_font, buf);
    if (blob)
      m_textLog.push_back(blob);
  }

  static std::string modifiersToString(KeyModifiers mods)
  {
    std::string s;
    if (mods & kKeyShiftModifier)
      s += " Shift";
    if (mods & kKeyCtrlModifier)
      s += " Ctrl";
    if (mods & kKeyAltModifier)
      s += " Alt";
    if (mods & kKeyCmdModifier)
      s += " Command";
    if (mods & kKeySpaceModifier)
      s += " Space";
    if (mods & kKeyWinModifier)
      s += " Win";
    return s;
  }

  SystemRef m_system;
  WindowRef m_window;
  FontMgrRef m_fontMgr;
  FontRef m_font;
  std::vector<TextBlobRef> m_textLog;
  size_t m_oldLogSize = 0;
  int m_lineHeight;
  int m_maxlines = 0;
  gfx::Point m_mousePos;
  double m_brushSize = 4;
  double m_hue = 0.0;
  bool m_nextEscCloses = false;
  bool m_textInput = true;
};

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(AppMode::GUI);

  LogWindow window(system);

  system->finishLaunching();
  system->activateApp();

  EventQueue* queue = system->eventQueue();
  while (true) {
    window.flush();

    Event ev;
    queue->getEvent(ev);
    if (!window.processEvent(ev))
      break;
  }

  return 0;
}
