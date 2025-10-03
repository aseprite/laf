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

#include "base/buffer.h"
#include "base/fs.h"
#include "base/string.h"

#include <shobjidl.h>
#include <windows.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace dlgs {

static std::string quote_arg(const std::string& in)
{
  std::string out;
  out.reserve(in.size() + 2);
  out.push_back('\"');
  for (auto chr : in) {
    // Add escape char '\' to double quotes
    if (chr == '\"')
      out.push_back('\\');
    out.push_back(chr);
  }
  // Generaly backslashes don't require a escape char, only if the
  // parameter ends with a backslash we have to add a escape.
  if (out.back() == '\\')
    out.push_back('\\');
  out.push_back('\"');
  return out;
}

// FileDialog impl for Windows calling the external laf-dlgs-proc
// executable. If the file dialog process crashes we don't kill the
// main executable.
class FileDialogWinSafe : public FileDialog {
  static constexpr const int kBufSize = MAX_PATH;

public:
  FileDialogWinSafe(const Spec& spec) : m_spec(spec) {}

  std::string fileName() override { return m_filename; }

  void getMultipleFileNames(base::paths& output) override { output = m_filenames; }

  void setFileName(const std::string& _filename) override
  {
    std::string filename = _filename;
    if (!filename.empty() && filename.back() != base::path_separator &&
        base::is_directory(filename)) {
      filename.push_back(base::path_separator);
    }
    m_filename = filename;
    m_lastPath = base::get_file_path(filename);
  }

  Result show(void* parent) override
  {
    Result result = Result::Error;

    std::string cmdLine = quote_arg(
      base::join_path(base::get_file_path(base::get_app_path()),
                      base::replace_extension(LAF_DLGS_PROC_NAME, "exe")));

    if (parent) {
      char buf[256];
      std::snprintf(buf, sizeof(buf), "0x%p", parent);
      cmdLine += " -parent ";
      cmdLine += buf;
    }

    cmdLine += " -type ";
    switch (m_type) {
      case Type::OpenFile:   cmdLine += "open"; break;
      case Type::OpenFiles:  cmdLine += "openfiles"; break;
      case Type::OpenFolder: cmdLine += "openfolder"; break;
      case Type::SaveFile:   cmdLine += "save"; break;
    }

    if (!m_title.empty())
      cmdLine += " -title " + quote_arg(m_title);

    if (!m_defExtension.empty())
      cmdLine += " -defaultextension " + quote_arg(m_defExtension);

    if (!m_filters.empty()) {
      for (const auto& kv : m_filters)
        cmdLine += " -addfilter " + quote_arg(kv.first + "=" + kv.second);
    }

    // Execute the laf-dlgs-proc as many times as crashes we receive
    // (in fact we use a "retry" count to stop at certain point).
    for (int retry = 0; retry < 100; ++retry) {
      std::string cmdLineWithFilename = cmdLine;
      if (!m_filename.empty())
        cmdLineWithFilename += " -filename " + quote_arg(m_filename);

      // Create a pipe for the STDOUT of the child process, so we can
      // receive its output
      SECURITY_ATTRIBUTES sa = {};
      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = nullptr;
      sa.bInheritHandle = TRUE;

      HANDLE childRead = nullptr;
      HANDLE childWrite = nullptr;
      if (!CreatePipe(&childRead, &childWrite, &sa, 0))
        break;

      STARTUPINFOW si = {};
      PROCESS_INFORMATION pi = {};
      si.cb = sizeof(si);
      si.hStdError = childWrite;
      si.hStdOutput = childWrite;
      si.hStdInput = INVALID_HANDLE_VALUE;
      si.dwFlags = STARTF_USESTDHANDLES;

      // CreateProcessW() needs a writable buffer, from:
      // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
      //
      //   "The Unicode version of this function, CreateProcessW, can
      //   modify the contents of this string. Therefore, this
      //   parameter cannot be a pointer to read-only memory (such as
      //   a const variable or a literal string). If this parameter is
      //   a constant string, the function may cause an access
      //   violation.
      //   [...]
      //   The system adds a terminating null character to the
      //   command-line string to separate the file name from the
      //   arguments. This divides the original string into two
      //   strings for internal processing."
      //
      // Anyway it looks like we can use the std::wstring data safely.

      std::wstring wcmdLine = base::from_utf8(cmdLineWithFilename);
      if (!CreateProcessW(nullptr,
                          (LPWSTR)wcmdLine.data(),
                          nullptr,
                          nullptr,
                          TRUE,             // bInheritHandles
                          DETACHED_PROCESS, // dwCreationFlags
                          nullptr,
                          nullptr,
                          &si,
                          &pi)) {
        break;
      }

      // Close handles that will not be used in this parent process
      CloseHandle(pi.hThread);
      CloseHandle(childWrite);

      // Start a thread to read the output of the process. Its output
      // include the last visited path and then the selected
      // filename(s).
      m_closingRead = false;
      std::thread readThread([this, childRead] { onReadChildDataThread(childRead); });

      // Wait the laf-dlgs-proc to finish (or crash)
      onWaitChildProcess((HWND)parent, pi.hProcess);

      // Wait the reader thread to finish.
      m_closingRead = true;
      readThread.join();

      // Did the process crash?
      DWORD exitCode = 0;
      GetExitCodeProcess(pi.hProcess, &exitCode);
      CloseHandle(pi.hProcess);
      if (exitCode == 0xC0000005 || !m_okReceived) {
        // Re-enable the window if the process crashes. It means that
        // IFileDialog impl wasn't able to end the process correctly
        // and re-enable the HWND, so we have to do this manually.
        EnableWindow((HWND)parent, TRUE);

        // Run child process again changing the last path (so we
        // restore the dialog in the same location the user was
        // browsing).
        m_filename = base::join_path(m_lastPath, base::get_file_name(m_filename));
        continue;
      }

      // Done (Cancel or OK)
      if (exitCode == 2) {
        result = FileDialog::Result::Error;
      }
      else if (exitCode == 1) {
        result = FileDialog::Result::Cancel;
      }
      else if (exitCode == 0) {
        result = FileDialog::Result::OK;
        if (!m_filenames.empty())
          m_filename = m_filenames.front();
      }
      break;
    }

    return result;
  }

  // This is executed in the main UI thread.
  void onWaitChildProcess(HWND parent, HANDLE childProcess)
  {
    // If there is no parent/main UI window, we can just wait the
    // child process to finish.
    if (!parent) {
      WaitForSingleObject(childProcess, INFINITE);
      return;
    }

    // If there is a main window, we have to create a message pump to
    // avoid locking the main window.
    MSG msg;
    while (!m_closingRead) {
      const DWORD res =
        MsgWaitForMultipleObjects(1, &childProcess, FALSE, 0, QS_ALLINPUT | QS_ALLPOSTMESSAGE);

      // The child process
      if (res == WAIT_OBJECT_0)
        break;
      else if (res == WAIT_OBJECT_0 + 1) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
    }
  }

  // This function is executed in a background thread to read the
  // output from laf-dlgs-proc process.
  int onReadChildDataThread(HANDLE childRead)
  {
    base::buffer buf(kBufSize, 0);
    bool startOver = true;
    std::string line;

    while (!m_closingRead) {
      buf[0] = 0;

      DWORD read = 0;
      BOOL result = ReadFile(childRead, buf.data(), buf.size(), &read, nullptr);
      if (!result || read == 0)
        break;

      line.clear();
      for (int i = 0; i < read; ++i) {
        if (startOver) {
          startOver = false;
          line.clear();
        }

        if (buf[i] == '\r')
          continue;
        if (buf[i] == '\n') {
          if (m_okReceived)
            m_filenames.push_back(line);
          else if (line == "OK")
            m_okReceived = true;
          else
            m_lastPath = line;

          startOver = true;
          continue;
        }
        line.push_back(buf[i]);
      }
    }
    return 0;
  }

  Spec m_spec;
  std::string m_filename;
  base::paths m_filenames;
  std::string m_initialDir;
  std::string m_lastPath;
  std::atomic<bool> m_closingRead = false;

  // True if the laf-dlgs-proc.exe signled the "OK" line.
  bool m_okReceived = false;
};

FileDialogRef FileDialog::makeWinSafe(const Spec& spec)
{
  return base::make_ref<FileDialogWinSafe>(spec);
}

} // namespace dlgs
