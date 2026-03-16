// LAF Base Library
// Copyright (c) 2021-2024 Igara Studio S.A.
// Copyright (c) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/process.h"

#include "base/fs.h"
#include "base/string.h"

#if LAF_WINDOWS
  #include <windows.h>
#else
  #include <signal.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#if LAF_MACOS && !LAF_IOS
  #include <libproc.h>
#endif

#if LAF_LINUX
  #include <cstdlib>
  #include <cstring>
#endif

namespace base {

#if LAF_WINDOWS

pid get_current_process_id()
{
  return (pid)GetCurrentProcessId();
}

bool is_process_running(pid pid)
{
  bool running = false;

  HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
  if (handle) {
    DWORD exitCode = 0;
    if (GetExitCodeProcess(handle, &exitCode)) {
      running = (exitCode == STILL_ACTIVE);
    }
    CloseHandle(handle);
  }

  return running;
}

std::string get_process_name(pid pid)
{
  std::string name;
  HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
  if (handle) {
    WCHAR exeName[MAX_PATH];
    DWORD len = MAX_PATH - 1;
    if (QueryFullProcessImageNameW(handle, 0, exeName, &len)) {
      name = base::get_file_name(base::to_utf8(exeName));
    }
    CloseHandle(handle);
  }
  return name;
}

#else

pid get_current_process_id()
{
  return (pid)getpid();
}

bool is_process_running(pid pid)
{
  return (kill(pid, 0) == 0);
}

  #if LAF_MACOS && !LAF_IOS

std::string get_process_name(pid pid)
{
  struct proc_bsdinfo process;
  proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &process, PROC_PIDTBSDINFO_SIZE);
  return process.pbi_name;
}

  #elif LAF_IOS

std::string get_process_name(pid pid)
{
  return "aseprite";
}

  #else

std::string get_process_name(pid pid)
{
  char path[128];
  std::memset(path, 0, sizeof(path));
  std::snprintf(path, sizeof(path), "/proc/%d/exe", pid);
  char* exepath = realpath(path, nullptr);
  if (!exepath)
    return std::string();

  std::string exename = base::get_file_name(exepath);
  free(exepath);

  return exename;
}

  #endif

#endif // LAF_WINDOWS

} // namespace base
