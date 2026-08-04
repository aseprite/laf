// LAF Library
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef LAF_EXAMPLES_TOGGLE_GPU_H_INCLUDED
#define LAF_EXAMPLES_TOGGLE_GPU_H_INCLUDED
#pragma once

#include "base/replace_string.h"
#include "os/event.h"
#include "os/window.h"

constexpr const char* kGpuSuffix = " (GPU)";

inline bool handle_toggle_gpu_key(const os::Event& ev)
{
  os::WindowRef w = ev.window();
  if (!w)
    return false;

  if (ev.type() != os::Event::KeyDown || ev.scancode() != os::kKeyF1)
    return false;

  w->setGpuAcceleration(!w->gpuAcceleration());
  w->invalidate();

  std::string title = w->title();
  base::replace_string(title, kGpuSuffix, {});
  w->setTitle(w->gpuAcceleration() ? title + kGpuSuffix : title);
  return true;
}

#endif
