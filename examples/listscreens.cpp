// LAF Library
// Copyright (c) 2020-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/os.h"

#include <cstdio>

using namespace os;

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(AppMode::CLI);

  ScreenList screens;
  system->listScreens(screens);

  bool unused_var;

  int i = 0;
  std::printf("Screens (%ld):\n", screens.size());
  for (auto screen : screens) {
    auto rc = screen->bounds();
    auto wa = screen->workarea();
    std::printf("%sscreen=%d bounds=(%d %d %d %d) workarea=(%d %d %d %d)\n",
                screen->isPrimary() ? "primary " : "",
                i++,
                rc.x,
                rc.y,
                rc.w,
                rc.h,
                wa.x,
                wa.y,
                wa.w,
                wa.h);
  }

  // crash?
  char* ptr = nullptr;
  *ptr = 1;

  return 0;
}
