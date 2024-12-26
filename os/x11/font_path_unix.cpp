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

#include <queue>

namespace app {

base::paths g_cache;

void get_font_dirs(base::paths& fontDirs)
{
  if (!g_cache.empty()) {
    fontDirs = g_cache;
    return;
  }

  std::queue<std::string> q;
  q.push("~/.fonts");
  q.push("/usr/local/share/fonts");
  q.push("/usr/share/fonts");

  while (!q.empty()) {
    std::string fontDir = q.front();
    q.pop();

    fontDirs.push_back(fontDir);

    for (const auto& file : base::list_files(fontDir, base::ItemType::Directories)) {
      std::string fullpath = base::join_path(fontDir, file);
      q.push(fullpath); // Add subdirectory in the queue
    }
  }

  g_cache = fontDirs;
}

} // namespace app
