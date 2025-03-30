// laf-dlgs
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifndef LAF_DLGS_PROC_NAME
  #error LAF_DLGS_PROC_NAME must be defined
#endif

#include "dlgs/file_dialog.h"

#include "base/fs.h"
#include "base/string.h"

#include <shobjidl.h>
#include <windows.h>

#include <string>
#include <vector>

namespace dlgs {

// FileDialog impl for Windows calling the external laf-dlgs-proc
// executable. If the file dialog process crashes we don't kill the
// main executable.
class FileDialogWinSafe : public FileDialog {
  static constexpr const int kBufSize = 4096;

public:
  FileDialogWinSafe(const Spec& spec) : m_spec(spec) {}

  std::string fileName() override { return m_filename; }

  void getMultipleFileNames(base::paths& output) override { output = m_filenames; }

  void setFileName(const std::string& filename) override
  {
    m_filename = filename;
    m_lastPath = base::from_utf8(base::get_file_path(filename));
  }

  Result show(void* parent) override
  {
    Result result = Result::Error;

    std::string cmdLine = base::join_path(base::get_file_path(base::get_app_path()),
                                          base::replace_extension(LAF_DLGS_PROC_NAME, "exe"));

    if (parent) {
      char buf[256];
      std::snprintf(buf, sizeof(buf), "0x%p", buf);
      cmdLine += " -parent ";
      cmdLine += buf;
    }

    if (!m_title.empty()) {
      cmdLine += " -title \"";
      cmdLine += m_title;
      cmdLine += "\"";
    }

    // Execute the laf-dlgs-proc as many times as crashes we receive
    // (in fact we use a "retry" count to stop at certain point).
    for (int retry = 0; retry < 100; ++retry) {
      // Create a pipe for the STDOUT of the child process, so we can
      // receive its output
      SECURITY_ATTRIBUTES sa = {};
      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = nullptr;
      sa.bInheritHandle = TRUE;

      m_childRead = nullptr;
      HANDLE childWrite = nullptr;
      if (!CreatePipe(&m_childRead, &childWrite, &sa, 0))
        break;

      STARTUPINFOW si = {};
      PROCESS_INFORMATION pi = {};
      si.cb = sizeof(si);
      si.hStdError = childWrite;
      si.hStdOutput = childWrite;
      si.hStdInput = INVALID_HANDLE_VALUE;
      si.dwFlags = STARTF_USESTDHANDLES;
      if (!CreateProcessW(nullptr,
                          (LPWSTR)base::from_utf8(cmdLine).c_str(),
                          nullptr,
                          nullptr,
                          TRUE,                                // bInheritHandles
                          CREATE_NO_WINDOW | DETACHED_PROCESS, // dwCreationFlags
                          nullptr,
                          nullptr,
                          &si,
                          &pi)) {
        break;
      }

      // Close handles that will not be used in this parent process
      CloseHandle(pi.hThread);
      CloseHandle(childWrite);

      HANDLE readThread =
        CreateThread(nullptr, 0, &FileDialogWinSafe::readChildDataThread, (LPVOID)this, 0, nullptr);

      // Wait the laf-dlgs-proc to finish (or crash)
      DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);

      // Did the process crash?
      DWORD exitCode = 0;
      GetExitCodeProcess(pi.hProcess, &exitCode);
      CloseHandle(pi.hProcess);
      if (exitCode == 0xC0000005) {
        // Run child process again
        continue;
      }

      // Done (Cancel or OK)
      if (exitCode == 1) {
        result = FileDialog::Result::Cancel;
      }
      else {
        result = FileDialog::Result::OK;
        m_filename = base::to_utf8(m_lastPath);
      }
      break;
    }

    return result;
  }

  // This function is executed in a background thread to read the
  // output from laf-dlgs-proc process.
  int onReadChildDataThread()
  {
    WCHAR buf[kBufSize];
    ZeroMemory(buf, sizeof(buf));

    while (TRUE) {
      buf[0] = 0;

      DWORD read = 0;
      BOOL result = ReadFile(m_childRead, buf, sizeof(buf), &read, nullptr);
      if (!result || read == 0)
        break;

      read /= 2; // As "buf" is WCHAR

      m_lastPath.clear();
      for (int i = 0; i < read; ++i) {
        if (buf[i] == '\r')
          continue;
        if (buf[i] == '\n') {
          m_lastPath.clear();
          continue;
        }
        m_lastPath.push_back(buf[i]);
      }
    }
    return 0;
  }

  static DWORD WINAPI readChildDataThread(LPVOID data)
  {
    auto* self = (FileDialogWinSafe*)data;
    return self->onReadChildDataThread();
  }

  Spec m_spec;
  std::string m_filename;
  base::paths m_filenames;
  std::string m_initialDir;
  HANDLE m_childRead = nullptr;
  std::wstring m_lastPath;
};

FileDialogRef FileDialog::makeWinSafe(const Spec& spec)
{
  return base::make_ref<FileDialogWinSafe>(spec);
}

} // namespace dlgs
