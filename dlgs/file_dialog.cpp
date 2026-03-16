// laf-dlgs
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dlgs/file_dialog.h"

namespace dlgs {

void FileDialog::setType(const Type type)
{
  m_type = type;
}

void FileDialog::setTitle(const std::string& title)
{
  m_title = title;
}

void FileDialog::setDefaultExtension(const std::string& extension)
{
  m_defExtension = extension;
}

void FileDialog::addFilter(const std::string& extension, const std::string& description)
{
  if (m_defExtension.empty())
    m_defExtension = extension;

  m_filters.push_back(std::make_pair(extension, description));
}

FileDialogRef FileDialog::make(const Spec& spec)
{
#if LAF_WINDOWS
  return FileDialog::makeWin(spec);
#elif LAF_IOS
  return FileDialog::makeIOS(spec);
#elif LAF_MACOS
  return FileDialog::makeOSX(spec);
#elif LAF_LINUX
  return FileDialog::makeX11(spec);
#else
  return nullptr;
#endif
}

} // namespace dlgs
