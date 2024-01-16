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
  #include <iostream>
  #include <tlhelp32.h>
#else
  #include <signal.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#if LAF_MACOS
  #include <libproc.h>
  #include <string.h>
#endif

#if LAF_LINUX
  #include "base/fs.h"
  #include <stdlib.h>
  #include <cstring>
#endif
namespace base {

#if LAF_WINDOWS

pid get_current_process_id()
{
  return (pid)GetCurrentProcessId();
}

bool is_process_running(pid pid, const char* pname)
{
  bool running = false;
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
        if (pe.th32ProcessID == pid &&
            str == pname) {
          running = true;
        }
      } while (Process32Next(handle, &pe));
    }
    CloseHandle(handle);
  }

  return running;
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

#elif LAF_MACOS

pid get_current_process_id()
{
  return (pid)getpid();
}

bool is_process_running(pid pid, const char* pname)
{
  struct proc_bsdinfo process;
  proc_pidinfo(pid, PROC_PIDTBSDINFO, 0,
               &process, PROC_PIDTBSDINFO_SIZE);
  return (strcmp(pname, process.pbi_name) == 0);
}

bool is_process_running(pid pid)
{
  return (kill(pid, 0) == 0);
}

#elif LAF_LINUX

pid get_current_process_id()
{
  return (pid)getpid();
}

bool is_process_running(pid pid, const char* pname)
{
  char path[128];
  memset(path, 0, 128);
  sprintf(path, "/proc/%d/exe", pid);
  char* exepath = realpath(path, nullptr);
  if (!exepath)
    return false;

  auto exename = base::get_file_name(exepath);
  free(exepath);

  return exename == std::string(pname);
}

bool is_process_running(pid pid)
{
  return (kill(pid, 0) == 0);
}

#endif

} // namespace base
