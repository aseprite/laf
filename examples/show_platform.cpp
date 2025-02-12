// LAF Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/platform.h"

#include <cstdio>

int app_main(int argc, char* argv[])
{
  base::Platform p = base::get_platform();
  switch (p.os) {
#if LAF_WINDOWS
    case base::Platform::OS::Windows:
      std::printf("Windows v%s%s%s",
                  p.osVer.str().c_str(),
                  p.isWow64 ? " WoW64" : "",
                  (p.wineVer ? (std::string(" WINE v") + p.wineVer).c_str() : ""));
      break;
#endif
    case base::Platform::OS::macOS: std::printf("macOS v%s", p.osVer.str().c_str()); break;
#if LAF_LINUX
    case base::Platform::OS::Linux:
      std::printf("Linux %s", p.distroName.c_str());
      if (!p.distroVer.empty())
        std::printf(" %s", p.distroVer.c_str());
      break;
#endif
  }
  switch (p.arch) {
    case base::Platform::Arch::x86:     std::printf(" (x86)"); break;
    case base::Platform::Arch::x64:     std::printf(" (x64)"); break;
    case base::Platform::Arch::arm64:   std::printf(" (arm64)"); break;
    case base::Platform::Arch::riscv64: std::printf(" (riscv64)"); break;
  }
  std::printf("\n");
  return 0;
}
