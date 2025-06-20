// laf-dlgs
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// Based in the work done in https://github.com/dacap/safedlgs/
// prototype.
//

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef LAF_DLGS_PROC_NAME
  #error LAF_DLGS_PROC_NAME must not be defined for laf-dlgs-proc
#endif

#include "base/program_options.h"
#include "base/string.h"
#include "base/win/coinit.h"
#include "dlgs/file_dialog.h"

#include <werapi.h>
#include <windows.h>

#include <cstdio>

class Delegate : public dlgs::FileDialogDelegate {
public:
  void onFolderChange(const std::string& path) override
  {
    // Print folder name in stdout.
    std::wprintf(L"%s\n", base::from_utf8(path).c_str());
    std::fflush(stdout);
  }
};

int wmain(int argc, wchar_t** wargv)
{
  base::ProgramOptions po;
  auto& parent(po.add("parent").requiresValue("<parent>"));
  auto& type(po.add("type").requiresValue("<open|save>"));
  auto& title(po.add("title").requiresValue("<title>"));
  auto& filename(po.add("filename").requiresValue("<filename>"));

  // Convert args to utf8 to parse them.
  {
    std::vector<std::string> argv_str(argc);
    for (int i = 0; i < argc; ++i)
      argv_str[i] = base::to_utf8(wargv[i]);

    std::vector<const char*> argv(argc);
    for (int i = 0; i < argc; ++i)
      argv[i] = argv_str[i].data();

    po.parse(argc, argv.data());
  }

  // Initialize COM library.
  base::CoInit com;

  // Avoid showing the "dlgs_process.exe has stopped working" dialog
  // when this crashes.
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
  WerSetFlags(WER_FAULT_REPORTING_NO_UI);

  Delegate delegate;
  dlgs::FileDialog::Spec spec;
  spec.delegate = &delegate;

  dlgs::FileDialogRef dlg = dlgs::FileDialog::makeWinUnsafe(spec);

  // TODO this doesn't work, it looks like we cannot use the parent
  // process HWND handle as parent of a file dialog in this other
  // process (?)
  HWND parentHandle = nullptr;
  {
    const std::string parentString = po.value_of(parent);
    if (!parentString.empty())
      parentHandle = (HWND)std::strtoull(parentString.c_str(), nullptr, 0);
  }

  if (po.value_of(type) == "save")
    dlg->setType(dlgs::FileDialog::Type::SaveFile);
  else
    dlg->setType(dlgs::FileDialog::Type::OpenFile);

  if (po.enabled(title))
    dlg->setTitle(po.value_of(title));

  if (po.enabled(filename))
    dlg->setFileName(po.value_of(filename));

  dlgs::FileDialog::Result result = dlg->show(parentHandle);
  if (result == dlgs::FileDialog::Result::Error)
    return 1;
  else if (result == dlgs::FileDialog::Result::Cancel)
    return 0;

  // Print file name
  std::wprintf(L"%s\n", base::from_utf8(dlg->fileName()).c_str());
  std::fflush(stdout);
  return 0;
}
