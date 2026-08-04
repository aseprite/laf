// LAF Library
// Copyright (C) 2022-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/os.h"

#ifndef LAF_SKIA
  #error You need Skia library to compile this example
#endif

#if !SK_SUPPORT_GPU
  #error You need Skia with GPU support to compile this example
#endif

#include "base/time.h"
#include "os/skia/skia_surface.h"

#include "examples/toggle_gpu.h"

#include "include/core/SkCanvas.h"
#include "include/effects/SkRuntimeEffect.h"

#include <cstdio>

using namespace os;

base::tick_t startTick;

// Shader from:
//   https://shaders.skia.org/
//   https://twitter.com/notargs/status/1250468645030858753
const char* shaderCode = R"(
uniform float3 iResolution;
uniform float  iTime;

float f(vec3 p) {
  p.z -= iTime * 10.0;
  float a = p.z * .1;
  p.xy *= mat2(cos(a), sin(a), -sin(a), cos(a));
  return .1 - length(cos(p.xy) + sin(p.yz));
}

half4 main(vec2 fragcoord) {
  vec3 d = .5 - fragcoord.xy1 / iResolution.y;
  vec3 p = vec3(0);
  for (int i = 0; i < 32; i++) {
    p += f(p) * d;
  }
  return ((sin(p) + vec3(2, 5, 9)) / length(p)).xyz1;
}
)";

class ShaderWindow {
public:
  ShaderWindow(const SystemRef& system)
    : m_system(system)
    , m_builder(SkRuntimeEffect::MakeForShader(SkString(shaderCode)).effect)
  {
    m_window = m_system->makeWindow(256, 256);
    m_window->setCursor(NativeCursor::Arrow);
    m_window->setTitle(std::string("Shader") + kGpuSuffix);
    m_window->setGpuAcceleration(true);
    repaint();
    m_window->setVisible(true);
  }

  bool processEvent(const Event& ev)
  {
    if (handle_toggle_gpu_key(ev))
      return true;

    switch (ev.type()) {
      case Event::CloseWindow:  return false;

      case Event::ResizeWindow: repaint(); break;

      case Event::KeyDown:
        if (ev.scancode() == kKeyEsc)
          return false;
        break;

      default:
        // Do nothing
        break;
    }
    return true;
  }

  void repaint()
  {
    Surface* surface = m_window->surface();
    SkCanvas* canvas = &static_cast<SkiaSurface*>(surface)->canvas();
    skiaPaint(canvas);

    m_window->invalidate();
    m_window->swapBuffers();
  }

private:
  void skiaPaint(SkCanvas* canvas)
  {
    SkImageInfo ii = canvas->imageInfo();
    m_builder.uniform("iResolution") = SkV3{ float(ii.width()), float(ii.height()), 0.0f };
    m_builder.uniform("iTime") = float((base::current_tick() - startTick) / 1000.0f);

    SkPaint p;
    p.setShader(m_builder.makeShader());
    canvas->drawPaint(p);
  }

  SystemRef m_system;
  WindowRef m_window;
  SkRuntimeShaderBuilder m_builder;
};

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(AppMode::GUI);

  ShaderWindow window(system);

  system->handleWindowResize = [&window](Window* win) { window.repaint(); };

  system->finishLaunching();
  system->activateApp();

  startTick = base::current_tick();

  EventQueue* queue = system->eventQueue();
  auto t = startTick;
  double paintDelay = 0.0;

  while (true) {
    Event ev;

    ASSERT(paintDelay >= 0.0);
    const double waitSecs = (base::current_tick() - t) / 1000.0 * 60.0 + paintDelay;

    queue->getEvent(ev, waitSecs);
    if (!window.processEvent(ev))
      break;

    auto now = base::current_tick();
    paintDelay -= (now - t) / 1000.0;
    if (paintDelay < 0.0) {
      auto paintStart = now;

      window.repaint();

      now = base::current_tick();
      paintDelay = (now - paintStart) / 1000.0;
    }
    t = now;
  }

  return 0;
}
