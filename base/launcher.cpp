// LAF Base Library
// Copyright (c) 2018-2021  Igara Studio S.A.
// Copyright (c) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/exception.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "base/string.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

#if !LAF_WINDOWS && defined(HAVE_SYSTEM)
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

#if LAF_WINDOWS
  #include <windows.h>
  #ifndef SEE_MASK_DEFAULT
    #define SEE_MASK_DEFAULT 0x00000000
  #endif

static int win32_shell_execute(const wchar_t* verb, const wchar_t* file, const wchar_t* params)
{
  SHELLEXECUTEINFO sh;
  ZeroMemory((LPVOID)&sh, sizeof(sh));
  sh.cbSize = sizeof(sh);
  sh.fMask = SEE_MASK_DEFAULT;
  sh.lpVerb = verb;
  sh.lpFile = file;
  sh.lpParameters = params;
  sh.nShow = SW_SHOWNORMAL;

  if (!ShellExecuteEx(&sh)) {
    int ret = GetLastError();
  #if 0
    if (ret != 0) {
      DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS;
      LPSTR msgbuf;

      if (FormatMessageA(flags, NULL, ret,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         reinterpret_cast<LPSTR>(&msgbuf),
                         0, NULL)) {
        ui::Alert::show("Problem<<Cannot open:<<%s<<%s||&Close", file, msgbuf);
        LocalFree(msgbuf);

        ret = 0;
      }
    }
  #endif
    return ret;
  }
  else
    return 0;
}
#endif // LAF_WINDOWS

namespace base { namespace launcher {

namespace {

bool starts_with_ci(const std::string& s, const char* prefix)
{
  const std::size_t n = std::strlen(prefix);
  if (s.size() < n)
    return false;
  for (std::size_t i = 0; i < n; ++i) {
    if (std::tolower(static_cast<unsigned char>(s[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i])))
      return false;
  }
  return true;
}

bool is_allowed_url(const std::string& url)
{
  if (url.empty())
    return false;
  for (unsigned char c : url) {
    if (c < 32 || c == '"' || c == '`' || c == '\\')
      return false;
  }
  return starts_with_ci(url, "https://") || starts_with_ci(url, "http://") ||
         starts_with_ci(url, "mailto:");
}

#if !LAF_WINDOWS && defined(HAVE_SYSTEM)
bool spawn_open(const char* tool, const char* arg1, const char* arg2 = nullptr)
{
  const pid_t pid = fork();
  if (pid < 0)
    return false;
  if (pid == 0) {
    setsid();
    if (arg2)
      execlp(tool, tool, arg1, arg2, static_cast<char*>(nullptr));
    else
      execlp(tool, tool, arg1, static_cast<char*>(nullptr));
    _exit(127);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0)
    return false;
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

} // namespace

bool open_url(const std::string& url)
{
  if (!is_allowed_url(url))
    return false;
  return open_file(url);
}

bool open_file(const std::string& file)
{
  if (file.empty() || file.find('\0') != std::string::npos)
    return false;

  int ret = -1;

#if LAF_WINDOWS

  ret = win32_shell_execute(L"open", base::from_utf8(file).c_str(), NULL);

#elif HAVE_SYSTEM

  #if __APPLE__
  ret = spawn_open("open", file.c_str()) ? 0 : -1;
  #else
  ret = spawn_open("xdg-open", file.c_str()) ? 0 : -1;
  #endif

#endif

  return (ret == 0);
}

bool open_folder(const std::string& _file)
{
  std::string file = base::fix_path_separators(_file);

#if LAF_WINDOWS

  int ret;
  if (base::is_directory(file)) {
    ret =
      win32_shell_execute(NULL, L"explorer", (L"/n,/e,\"" + base::from_utf8(file) + L"\"").c_str());
  }
  else {
    ret = win32_shell_execute(NULL,
                              L"explorer",
                              (L"/e,/select,\"" + base::from_utf8(file) + L"\"").c_str());
  }
  return (ret == 0);

#elif HAVE_SYSTEM

  if (file.empty() || file.find('\0') != std::string::npos)
    return false;

  #if __APPLE__

  if (base::is_directory(file))
    return spawn_open("open", file.c_str());
  return spawn_open("open", "--reveal", file.c_str());

  #else

  if (!base::is_directory(file))
    file = base::get_file_path(file);

  return spawn_open("xdg-open", file.c_str());

  #endif

#else // HAVE_SYSTEM

  return false;

#endif
}

}} // namespace base::launcher
