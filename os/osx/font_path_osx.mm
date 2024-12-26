// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/font_path.h"

#include "base/fs.h"

#include <cstdlib>

namespace os {

// TODO use a Cocoa API to get the list of paths
void get_font_dirs(base::paths& fontDirs)
{
  const char* home = std::getenv("HOME");
  if (home) {
    fontDirs.push_back(base::join_path(home, "Library/Fonts"));
  }
  fontDirs.push_back("/Library/Fonts");
  fontDirs.push_back("/System/Library/Fonts/");
}

} // namespace os
