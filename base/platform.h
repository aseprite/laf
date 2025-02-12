// LAF Base Library
// Copyright (c) 2021  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_PLATFORM_H_INCLUDED
#define BASE_PLATFORM_H_INCLUDED
#pragma once

#include "base/version.h"

#include <cstdint>

#if LAF_LINUX
  #include <map>
  #include <string>
#endif

namespace base {

struct Platform {
  enum class OS {
    Unknown,
    Windows,
    macOS,
    Linux,
  };

  enum class Arch {
    x86,     // Windows 32-bit, Linux 32-bit
    x64,     // Windows 64-bit, Mac Intel
    arm64,   // Mac Apple Silicon (M1, M1 Pro), Raspberry Pi?
    riscv64, // Linux RISC-V 64-bit
  };

  static constexpr OS os =
#if LAF_WINDOWS
    OS::Windows
#elif LAF_MACOS
    OS::macOS
#else
    OS::Linux
#endif
    ;

  static constexpr Arch arch =
#if defined(__arm64__) || defined(__aarch64__)
    Arch::arm64
#elif defined(__riscv) && __riscv_xlen == 64
    Arch::riscv64
#elif defined(__x86_64__) || defined(_WIN64)
    Arch::x64
#else
    Arch::x86
#endif
    ;

  static_assert((arch == Arch::x86 && sizeof(void*) == 4) ||
                  ((arch == Arch::x64 || arch == Arch::arm64 || arch == Arch::riscv64) &&
                   sizeof(void*) == 8),
                "Invalid identification of CPU architecture");

  Version osVer;

#if LAF_WINDOWS
  enum class WindowsType { Unknown, Server, NT };
  WindowsType windowsType = WindowsType::Unknown;
  Version servicePack;           // Service pack version
  bool isWow64 = false;          // 32-bit version running on 64-bit
  const char* wineVer = nullptr; // Are we running on Wine emulator?
#elif LAF_MACOS
    // Nothing extra (macOS version on "osVer" field)
#elif LAF_LINUX
  std::string distroName; // Linux distribution name
  std::string distroVer;  // Distribution version
#endif
};

Platform get_platform();

#if LAF_WINDOWS

bool is_wow64();
const char* get_wine_version();

#elif LAF_MACOS

Version get_osx_version();

#elif LAF_LINUX

// Reads all the information of the current Linux distro from
// /etc/os-release or /etc/lsb-release (you specify the filename)
std::map<std::string, std::string> get_linux_release_info(const std::string& fn);

#endif

} // namespace base

#endif
