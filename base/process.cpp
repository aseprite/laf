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

#if LAF_WINDOWS
  #include <windows.h>
  #include <tlhelp32.h>
#else
  #include <signal.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#if LAF_MACOS
  #include <libproc.h>
#elif LAF_LINUX
  #include "base/fs.h"
  #include <cstring>
#endif

namespace base {

#if LAF_WINDOWS

pid get_current_process_id()
{
  return (pid)GetCurrentProcessId();
}

std::string get_process_name(pid pid)
{
  HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
  if (handle) {
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(handle, &pe)) {
      do {
        char buf[64];
        wcstombs(buf, pe.szExeFile, 64);
        std::string str(buf);
        for (char& c : str) {
          c = tolower(c);
        }
        if (pe.th32ProcessID == pid) {
          return str;
        }
      } while (Process32Next(handle, &pe));
    }
    CloseHandle(handle);
  }
  return "";
}

#elif LAF_MACOS

pid get_current_process_id()
{
  return (pid)getpid();
}

std::string get_process_name(pid pid)
{
  struct proc_bsdinfo process;
  proc_pidinfo(pid, PROC_PIDTBSDINFO, 0,
               &process, PROC_PIDTBSDINFO_SIZE);
  return process.pbi_name;
}

#elif LAF_LINUX

pid get_current_process_id()
{
  return (pid)getpid();
}

std::string get_process_name(pid pid)
{
  char path[128];
  memset(path, 0, 128);
  sprintf(path, "/proc/%d/exe", pid);
  char* exepath = realpath(path, nullptr);
  if (!exepath)
    return "";

  auto exename = base::get_file_name(exepath);
  free(exepath);

  return exename;
}

#endif

bool is_process_running(pid pid, std::string currentProcessName)
{
  std::string pidProcessName = get_process_name(pid);
  if (pidProcessName == "")
    return false;
  return pidProcessName == currentProcessName;
}

bool is_process_running(pid pid)
{
  return false;
};

} // namespace base
