// LAF Base Library
// Copyright (c) 2026-present Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_FILE_SIZE_H_INCLUDED
#define BASE_FILE_SIZE_H_INCLUDED
#pragma once

#include "base/ints.h"

#include <cstdio>
#include <string>

namespace base {

// * On Windows _ftelli64/_fseeki64 uses a __int64,
//   and _stat64.st_size is __int64 (used by _wstat64)
// * On macOS off_t = __darwin_off_t = __int64
// * On Linux ftello/fseeko use off_t which is an int64_t
// * On C++17 std::file_system::file_size() returns an uintmax_t, but in
//   case of non-throwing error it returns static_cast<std::uintmax_t>(-1)
//   so it looks like the signed version makes more sense
//
// For those reasons we prefer a signed file size (int64_t).
using filesize_t = intmax_t;
using fileoff_t = intmax_t;

fileoff_t base_ftell(FILE* file);
int base_fseek(FILE* file, fileoff_t offset, int origin);
fileoff_t base_lseek(int fd, fileoff_t offset, int origin);

filesize_t file_size(FILE* file);
filesize_t file_size(const std::string& path);

} // namespace base

#endif
