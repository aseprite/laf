// LAF Base Library
// Copyright (c) 2021-2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/platform.h"

#if LAF_WINDOWS
  #include <windows.h>

typedef LONG(WINAPI* RtlGetVersion_Func)(OSVERSIONINFOEX*);
#elif LAF_LINUX // Unix-like system
  #include <sys/utsname.h>
#endif

namespace base {

Platform get_platform()
{
  Platform p;

#if LAF_WINDOWS

  // ----------------------------------------------------------------------
  // Windows

  OSVERSIONINFOEX osv;
  osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

  // It looks like GetVersionEx() returns 6.2 for all Windows versions
  // after Windows 8 and RtlGetVersion() returns the correct version
  // and build number for Windows 10 and 11.
  static const auto fnRtlGetVersion = reinterpret_cast<RtlGetVersion_Func>(
    GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion"));
  if (!fnRtlGetVersion || !fnRtlGetVersion(&osv) || !osv.dwMajorVersion) {
    OSVERSIONINFOEX osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    // Ignore deprecated function warning
  #pragma warning(push)
  #pragma warning(disable : 4996)
    GetVersionEx((OSVERSIONINFO*)&osv);
  #pragma warning(pop)
  }

  p.osVer = Version(osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, 0);
  switch (osv.wProductType) {
    case VER_NT_DOMAIN_CONTROLLER:
    case VER_NT_SERVER:            p.windowsType = Platform::WindowsType::Server; break;
    case VER_NT_WORKSTATION:       p.windowsType = Platform::WindowsType::NT; break;
  }

  p.servicePack = Version(osv.wServicePackMajor, osv.wServicePackMinor, 0, 0);
  p.isWow64 = is_wow64();
  p.wineVer = get_wine_version();

#elif LAF_IOS

  // ----------------------------------------------------------------------
  // iOS

  p.osVer = get_ios_version();

#elif LAF_MACOS

  // ----------------------------------------------------------------------
  // Mac OS X/macOS

  p.osVer = get_osx_version();

#elif LAF_LINUX

  // ----------------------------------------------------------------------
  // Unix like

  #define HAS_VALUE(n) ((n) != values.end() && !(n)->second.empty())

  // Read information from /etc/os-release
  auto values = get_linux_release_info("/etc/os-release");
  auto name = values.find("PRETTY_NAME");
  if (HAS_VALUE(name)) {
    p.distroName = name->second;
  }
  else {
    name = values.find("NAME");
    auto ver = values.find("VERSION");
    if (HAS_VALUE(name)) {
      p.distroName = name->second;
      if (HAS_VALUE(ver))
        p.distroVer = ver->second;
    }
    else {
      // Read information from /etc/lsb-release
      values = get_linux_release_info("/etc/lsb-release");
      name = values.find("DISTRIB_DESCRIPTION");
      if (HAS_VALUE(name)) {
        p.distroName = name->second;
      }
      else {
        name = values.find("DISTRIB_ID");
        ver = values.find("DISTRIB_RELEASE");
        if (HAS_VALUE(name)) {
          p.distroName = name->second;
          if (HAS_VALUE(ver))
            p.distroVer = ver->second;
        }
        else {
          // Last resource, use uname() function
          struct utsname utsn;
          uname(&utsn);
          p.distroName = utsn.sysname;
          p.distroVer = utsn.release;
        }
      }
    }
  }

#endif

  return p;
}

} // namespace base
