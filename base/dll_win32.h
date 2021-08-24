// LAF Base Library
// Copyright (c) 2020 Igara Studio S.A.
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/debug.h"
#include "base/fs.h"
#include "base/string.h"
#include "base/version.h"

#include <windows.h>

namespace base {

dll load_dll(const std::string& filename)
{
  return LoadLibrary(base::from_utf8(filename).c_str());
}

void unload_dll(dll lib)
{
  FreeLibrary((HMODULE)lib);
}

dll_proc get_dll_proc_base(dll lib, const char* procName)
{
  return reinterpret_cast<dll_proc>(
    GetProcAddress((HMODULE)lib, procName));
}

static bool get_dll_filename_wchar(dll lib, std::vector<wchar_t>& buf)
{
  while (true) {
    DWORD newSize = GetModuleFileNameW((HMODULE)lib, &buf[0], (DWORD)buf.size());
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
      ASSERT(buf.size() < newSize); // it cannot fail (?)
      buf.resize(newSize);
    }
    else
      return GetLastError() == ERROR_SUCCESS;
  }
}

std::string get_dll_filename(dll lib)
{
  std::vector<wchar_t> buf(MAX_PATH);
  if (get_dll_filename_wchar(lib, buf) &&
      buf.size() > 1)           // One char for the null char
    return to_utf8(&buf[0]);
  else
    return std::string();
}

Version get_dll_version(dll lib)
{
  std::vector<wchar_t> buf(MAX_PATH);
  if (get_dll_filename_wchar(lib, buf))
    return get_file_version(&buf[0]);
  else
    return Version();
}

} // namespace base
