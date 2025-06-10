// LAF OS Library
// Copyright (C) 2018-2020  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/osx/color_space.h"

#include "base/file_content.h"
#include "base/fs.h"
#include "base/string.h"
#include "os/system.h"

#include <unordered_map>

#include <windows.h>

namespace os {

std::unordered_map<HMONITOR, std::string> g_iccFilenameCache;
std::unordered_map<std::string, os::ColorSpaceRef> g_colorspaceCache;

std::string get_hmonitor_icc_filename(HMONITOR monitor)
{
  const auto& it = g_iccFilenameCache.find(monitor);
  if (it != g_iccFilenameCache.end()) {
    return it->second;
  }

  std::string iccFilename;
  MONITORINFOEX mi;
  ZeroMemory(&mi, sizeof(MONITORINFOEX));
  mi.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(monitor, &mi);
  HDC hdc = CreateDC(mi.szDevice, nullptr, nullptr, nullptr);
  if (hdc) {
    DWORD length = MAX_PATH;
    std::vector<TCHAR> str(length + 1);
    if (GetICMProfile(hdc, &length, &str[0])) {
      iccFilename = base::to_utf8(&str[0]);
      g_iccFilenameCache.emplace(monitor, iccFilename);
    }
    DeleteDC(hdc);
  }
  return iccFilename;
}

os::ColorSpaceRef get_colorspace_from_icc_file(const std::string& iccFilename)
{
  const auto& it = g_colorspaceCache.find(iccFilename);
  if (it != g_colorspaceCache.end()) {
    return it->second;
  }

  auto system = System::instance();
  ASSERT(system);
  if (!system)
    return nullptr;

  auto buf = base::read_file_content(iccFilename);
  auto osCS = system->makeColorSpace(gfx::ColorSpace::MakeICC(std::move(buf)));
  if (osCS) {
    osCS->gfxColorSpace()->setName("Display Profile: " + base::get_file_title(iccFilename));
    g_colorspaceCache.emplace(iccFilename, osCS);
  }
  return osCS;
}

os::ColorSpaceRef get_hmonitor_colorspace(HMONITOR monitor)
{
  os::ColorSpaceRef osCS;
  std::string filename = get_hmonitor_icc_filename(monitor);
  if (!filename.empty())
    osCS = get_colorspace_from_icc_file(filename);
  return osCS;
}

static BOOL CALLBACK list_display_colorspaces_enumproc(HMONITOR monitor,
                                                       HDC hdc,
                                                       LPRECT rc,
                                                       LPARAM data)
{
  auto list = (std::vector<os::ColorSpaceRef>*)data;
  auto osCS = get_hmonitor_colorspace(monitor);
  if (osCS)
    list->push_back(osCS);
  return TRUE;
}

void list_display_colorspaces(std::vector<os::ColorSpaceRef>& list)
{
  EnumDisplayMonitors(nullptr, nullptr, list_display_colorspaces_enumproc, (LPARAM)&list);
}

} // namespace os
