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

namespace os {

std::string find_font(const std::string& firstDir, const std::string& filename)
{
  std::string fn = base::join_path(firstDir, filename);
  if (base::is_file(fn))
    return fn;

  base::paths fontDirs;
  get_font_dirs(fontDirs);

  for (const std::string& dir : fontDirs) {
    fn = base::join_path(dir, filename);
    if (base::is_file(fn))
      return fn;
  }

  return std::string();
}

} // namespace os
