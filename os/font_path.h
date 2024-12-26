// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_FONT_PATH_H_INCLUDED
#define OS_FONT_PATH_H_INCLUDED
#pragma once

#include "base/paths.h"

#include <string>

namespace os {

void get_font_dirs(base::paths& fontDirs);
std::string find_font(const std::string& firstDir, const std::string& filename);

} // namespace os

#endif
