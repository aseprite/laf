// LAF OS Library
// Copyright (C) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/x11/native_dialogs.h"

#include "base/file_handle.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/replace_string.h"
#include "base/split_string.h"
#include "os/common/file_dialog.h"
#include "os/x11/x11.h"

#include <cstdio>              // popen/pclose()
#include <cstring>

namespace os {

static std::string quote_for_shell(const std::string& in)
{
  std::string out;
  out.reserve(in.size()+2);
  out.push_back('\"');
  for (auto chr : in) {
    // Add escape char '\' to double quotes and backslashes
    if (chr == '\"' ||
        chr == '\\') {
      out.push_back('\\');
      out.push_back(chr);
    }
    else {
      out.push_back(chr);
    }
  }
  out.push_back('\"');
  return out;
}

// Uses zenity or kdialog to display the native file dialog.
class FileDialogX11 : public CommonFileDialog {
public:
  enum class CLITool {
    Unknown,
    NotFound,
    Zenity,                     // Used for GNOME/GTK+
    KDialog,                    // Used for KDE
  };

  FileDialogX11() {
  }

  std::string fileName() override {
    return m_filename;
  }

  void getMultipleFileNames(base::paths& output) override {
    output = m_filenames;
  }

  void setFileName(const std::string& filename) override {
    m_filename = filename;
    if (base::is_directory(m_filename))
      m_initialDir = m_filename;
    else {
      m_initialDir = base::get_file_path(m_filename);

      // TODO Zenity doesn't have a proper support to add the default
      // extension when the user confirms the filename, or when the
      // filter is changed later on. There is an issue related to
      // this in their repo:
      //
      //   https://gitlab.gnome.org/GNOME/zenity/-/issues/27
      //
      // The "best" way to mitigate this is to set the default
      // extension to the filename itself (if it doesn't have one),
      // and then ignore the file filter option (if it's changed it'll
      // only for filtering purposes, but the filename keeps the
      // initial specified extension until the user change/rewrite
      // it).
      if (base::get_file_extension(m_filename).empty())
        m_filename = base::replace_extension(m_filename, m_defExtension);
    }
  }

  Result show(Window* parent) override {
    switch (s_cliTool) {

      case CLITool::Zenity: {
        std::string cmd;
        cmd = "zenity --file-selection --title " + quote_for_shell(m_title);

        if (!m_filters.empty()) {
          // A filter for all supported file formats (*.extension1 *.extension2 etc.)
          cmd += " \"--file-filter=All formats|";
          for (const auto& kv : m_filters) {
            cmd += " *.";
            cmd += kv.first;
          }
          cmd += "\"";
          // One filter for each file format
          for (const auto& kv : m_filters) {
            cmd += " \"--file-filter=";
            cmd += kv.second;
            cmd += " | *.";
            cmd += kv.first;
            cmd += "\"";
          }
        }
        // A filter for all files (*.*)
        cmd += " \"--file-filter=All files|*.*\"";

        if (m_type == Type::SaveFile) {
          cmd += " --save --confirm-overwrite";
          if (!m_filename.empty())
            cmd += " --filename " + quote_for_shell(m_filename);
        }
        else {
          // Initial directory
          if (!m_initialDir.empty())
            cmd += " --filename " + quote_for_shell(base::join_path(m_initialDir, "file"));

          if (m_type == Type::OpenFiles)
            cmd += " --multiple";
        }

        // TODO There is an feature request in Zenity to add
        // the --print-winid parameter:
        //
        //   https://gitlab.gnome.org/GNOME/zenity/-/issues/13
        //
        // This option is supported by KDialog, and might be useful to
        // specify the parent relationship (XSetTransientForHint)
        // between the given "parent" argument in this show() function
        // and the created window by the utility.

        // Flushes pending events to the X Server, so that input does not
        // get stuck when opening a pipe to zenity.
        XFlush(X11::instance()->display());
        // Here we run the command and get a handle to read its
        // stdout.
        FILE* f = popen(cmd.c_str(), "r");
        if (!f) {
          // Problem calling native dialog, Result::Error is returned
          // and a custom file selector should be used.
          break;
        }

        // Read the stdout of the command
        std::string allFiles;
        char buf[512];
        while (fgets(buf, sizeof(buf), f)) {
          if (buf[0] == '\n' && !buf[1])
            continue;
          else if (buf[0]) {
            int n = std::strlen(buf);
            while (buf[0] && std::isspace(buf[n-1])) {
              buf[n-1] = 0;
              --n;
            }
            allFiles += buf;
          }
        }

        int ret = pclose(f);
        switch (ret) {
          case 0:
            m_filenames.clear();
            if (m_type == Type::OpenFiles) {
              m_filename.clear();

              base::split_string(allFiles, m_filenames, "|");
              if (!m_filenames.empty())
                m_filename = m_filenames[0];
            }
            else {
              m_filename = allFiles;
            }
            return Result::OK;
          case 1:
          case 256:
            return Result::Cancel;
          default:
            LOG(ERROR, "Error running zenity command %d", ret);
            break;
        }
        break;
      }

      case CLITool::KDialog:
        // TODO
        break;

      default:
        break;
    }

    return Result::Error;
  }

  static bool AreCLIToolsAvailable() {
    if (s_cliTool == CLITool::Unknown) {
      FILE* f = popen("zenity --version", "r");
      if (f && pclose(f) == 0) {
        s_cliTool = CLITool::Zenity;
      }
      else {
        f = popen("kdialog --version", "r");
        if (f && pclose(f) == 0)
          s_cliTool = CLITool::KDialog;
        else
          s_cliTool = CLITool::NotFound;
      }
    }
    return (s_cliTool > CLITool::NotFound);
  }

private:
  std::string m_filename;
  base::paths m_filenames;
  std::string m_initialDir;
  static CLITool s_cliTool;
};

FileDialogX11::CLITool FileDialogX11::s_cliTool =
  FileDialogX11::CLITool::Unknown;

NativeDialogsX11::NativeDialogsX11()
{
}

FileDialogRef NativeDialogsX11::makeFileDialog()
{
  if (FileDialogX11::AreCLIToolsAvailable())
    return make_ref<FileDialogX11>();
  else
    return nullptr;
}

} // namespace os
