// LAF OS Library
// Copyright (C) 2021  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/memory.h"
#include "base/string.h"

#if LAF_WINDOWS
  #include <windows.h>
#elif LAF_MACOS
  #include "os/osx/app.h"
#elif LAF_LINUX
  #include "os/x11/x11.h"
#endif

extern int app_main(int argc, char* argv[]);

#if LAF_WINDOWS
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR lpCmdLine, int nCmdShow) {
  int argc = __argc;
  char** argv;
  if (__wargv && argc > 0) {
    argv = new char*[argc];
    for (int i=0; i<argc; ++i)
      argv[i] = base_strdup(base::to_utf8(std::wstring(__wargv[i])).c_str());
  }
  else {
    argv = new char*[1];
    argv[0] = base_strdup("");
    argc = 1;
  }
  return app_main(argc, argv);
}

int wmain(int argc, wchar_t* wargv[], wchar_t* envp[]) {
  char** argv;
  if (wargv && argc > 0) {
    argv = new char*[argc];
    for (int i=0; i<argc; ++i)
      argv[i] = base_strdup(base::to_utf8(std::wstring(wargv[i])).c_str());
  }
  else {
    argv = new char*[1];
    argv[0] = base_strdup("");
    argc = 1;
  }
  return app_main(argc, argv);
}
#endif

int main(int argc, char* argv[]) {
#if LAF_MACOS
  os::AppOSX app;
  if (!app.init())
    return 1;
#elif LAF_LINUX
  os::X11 x11();
#endif
  return app_main(argc, argv);
}
