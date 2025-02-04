// laf-dlgs
// Copyright (C) 2024-2025  Igara Studio S.A.
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
#include "base/split_string.h"
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
    std::printf("%s\\\n", path.c_str());
    std::fflush(stdout);
  }
};

int wmain(int argc, wchar_t** wargv)
{
  base::ProgramOptions po;
  auto& parent(po.add("parent").requiresValue("<parent>"));
  auto& type(po.add("type").requiresValue("<open|openfiles|openfolder|save>"));
  auto& title(po.add("title").requiresValue("<title>"));
  auto& filename(po.add("filename").requiresValue("<filename>"));
  auto& defaultExtension(po.add("defaultextension").requiresValue("<extension>"));
  auto& addFilter(po.add("addfilter").requiresValue("<extension>;<description>"));

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

  // Get the parent HWND from the arguments.
  HWND parentHandle = nullptr;
  {
    const std::string parentString = po.value_of(parent);
    if (!parentString.empty())
      parentHandle = (HWND)std::strtoull(parentString.c_str(), nullptr, 0);
  }

  bool multipleFiles = false;
  if (po.value_of(type) == "save")
    dlg->setType(dlgs::FileDialog::Type::SaveFile);
  else if (po.value_of(type) == "openfiles") {
    dlg->setType(dlgs::FileDialog::Type::OpenFiles);
    multipleFiles = true;
  }
  else if (po.value_of(type) == "openfolder")
    dlg->setType(dlgs::FileDialog::Type::OpenFolder);
  else
    dlg->setType(dlgs::FileDialog::Type::OpenFile);

  if (po.enabled(title))
    dlg->setTitle(po.value_of(title));

  if (po.enabled(filename))
    dlg->setFileName(po.value_of(filename));

  if (po.enabled(defaultExtension))
    dlg->setDefaultExtension(po.value_of(defaultExtension));

  for (auto v : po.values()) {
    if (v.option() == &addFilter) {
      std::vector<std::string> result;
      base::split_string(v.value(), result, "=");
      if (result.size() == 2)
        dlg->addFilter(result[0], result[1]);
      else if (result.size() == 1)
        dlg->addFilter(result[0], result[0]);
    }
  }

  dlgs::FileDialog::Result result = dlg->show(parentHandle);

  // Print a "OK" signal so the parent process knowns that native
  // show() impl didn't crash.
  std::printf("OK\n");

  if (result == dlgs::FileDialog::Result::Error)
    return 2;
  if (result == dlgs::FileDialog::Result::Cancel)
    return 1;

  // Print file name(s)
  if (multipleFiles) {
    base::paths files;
    dlg->getMultipleFileNames(files);
    for (auto fn : files)
      std::printf("%s\n", fn.c_str());
  }
  else
    std::printf("%s\n", dlg->fileName().c_str());

  std::fflush(stdout);
  return 0;
}
