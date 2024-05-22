// LAF Base Library
// Copyright (c) 2020-2024 Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_STRING_H_INCLUDED
#define BASE_STRING_H_INCLUDED
#pragma once

#include <cstdarg>
#include <iterator>
#include <string>

namespace base {

  std::string string_printf(const char* format, ...);
  std::string string_vprintf(const char* format, std::va_list ap);

  std::string string_to_lower(const std::string& original);
  std::string string_to_upper(const std::string& original);
  std::string removeEscapeCharFromText(const std::string& original,
                                       const int escapeChar);
  
  std::string to_utf8(const wchar_t* src, size_t n);

  inline std::string to_utf8(const std::wstring& widestring) {
   return to_utf8(widestring.c_str(), widestring.size());
  }

  std::wstring from_utf8(const std::string& utf8string);

  int utf8_length(const std::string& utf8string);
  int utf8_icmp(const std::string& a, const std::string& b, int n = 0);

}

#endif
