// laf-dlgs
// Copyright (c) 2020-2024  Igara Studio S.A.
// Copyright (c) 2015-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DLGS_FILE_DIALOG_H_INCLUDED
#define DLGS_FILE_DIALOG_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "base/ref.h"

#include <string>
#include <utility>
#include <vector>

namespace dlgs {
class FileDialog;
using FileDialogRef = base::Ref<FileDialog>;

#if LAF_WINDOWS
class FileDialogDelegate {
public:
  virtual ~FileDialogDelegate() {}
  virtual void onFolderChange(const std::string& path) = 0;
};
#endif // LAF_WINDOWS

class FileDialog : public base::RefCount {
public:
  enum class Type {
    OpenFile,
    OpenFiles,
    OpenFolder,
    SaveFile,
  };

  enum class Result {
    Error = -1, // This happens when we cannot open the native dialog
    Cancel = 0, // The user canceled the dialog
    OK = 1,     // The user selected some file(s)
  };

  struct Spec {
#if LAF_WINDOWS
    // Listen events of the FileDialog.
    FileDialogDelegate* delegate = nullptr;
#elif LAF_MACOS
    // Indicates which is the "Edit" menu (NSMenuItem*) with
    // Undo/Redo/Cut/Copy/Paste/etc. commands. Used by the
    // FileDialogOSX impl to completely replace the "Edit" menu with a
    // standard one to make these
    // Undo/Redo/Cut/Copy/Paste/etc. keyboard shortcuts work.
    //
    // The specific details are not beautiful or important, but you
    // can search for "OSXEditMenuHack" to know why this is needed.
    void* editNSMenuItem = nullptr;
#endif

#if LAF_LINUX
    // Connection to the X11 server (the Display* pointer returned by
    // XOpenDisplay()).
    void* x11display = nullptr;
#endif
  };

  static FileDialogRef make(const Spec& spec);
#if LAF_WINDOWS
  static FileDialogRef makeWin(const Spec& spec);
  static FileDialogRef makeWinUnsafe(const Spec& spec);
  #ifdef LAF_DLGS_PROC_NAME
  static FileDialogRef makeWinSafe(const Spec& spec);
  #endif
#elif LAF_MACOS
  static FileDialogRef makeOSX(const Spec& spec);
#elif LAF_LINUX
  static FileDialogRef makeX11(const Spec& spec);
#endif

  virtual ~FileDialog() {}

  void setType(const Type type);
  void setTitle(const std::string& title);
  void setDefaultExtension(const std::string& extension);
  void addFilter(const std::string& extension, const std::string& description);

  virtual std::string fileName() = 0;
  virtual void getMultipleFileNames(base::paths& output) = 0;
  virtual void setFileName(const std::string& filename) = 0;

  // The native window handle is an HWND on Windows, a NSWindow* on
  // macOS, or an X11 Window on Linux.
  virtual Result show(void* windowNative) = 0;

protected:
  Type m_type = Type::OpenFile;
  std::string m_title;
  std::string m_defExtension;
  std::vector<std::pair<std::string, std::string>> m_filters;
};

} // namespace dlgs

#endif
