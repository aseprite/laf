// LAF Base Library
// Copyright (C) 2018-2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_FILE_CONTENT_H_INCLUDED
#define BASE_FILE_CONTENT_H_INCLUDED
#pragma once

#include "base/buffer.h"
#include "base/ints.h"

#include <cstdio>
#include <string>

namespace base {

  buffer read_file_content(FILE* file);
  buffer read_file_content(const std::string& filename);

  void write_file_content(FILE* file, const uint8_t* data, size_t size);
  void write_file_content(const std::string& filename, const uint8_t* data, size_t size);

  inline void write_file_content(FILE* file, const buffer& buf) {
    if (!buf.empty())
      write_file_content(file, &buf[0], buf.size());
  }

  inline void write_file_content(const std::string& filename, const buffer& buf) {
    if (!buf.empty())
      write_file_content(filename, &buf[0], buf.size());
  }

  // Can be used on Windows to write binary content to stdout or other
  // FILE handles.
  void set_write_binary_file_content(FILE* file);

} // namespace base

#endif
