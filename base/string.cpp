// LAF Base Library
// Copyright (c) 2020-2021 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/debug.h"
#include "base/string.h"

#include <cctype>
#include <vector>
#include <codecvt>

namespace base {

std::string string_printf(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);
  std::string result = string_vprintf(format, ap);
  va_end(ap);
  return result;
}

std::string string_vprintf(const char* format, va_list ap)
{
  std::vector<char> buf(1, 0);
  std::va_list ap2;
  va_copy(ap2, ap);
  size_t required_size = std::vsnprintf(nullptr, 0, format, ap);
  if (required_size) {
    buf.resize(required_size+1);
    std::vsnprintf(&buf[0], buf.size(), format, ap2);
  }
  va_end(ap2);
  return std::string(&buf[0]);
}

std::string string_to_lower(const std::string& original)
{
  std::wstring result(from_utf8(original));
  auto it(result.begin());
  auto end(result.end());
  while (it != end) {
    *it = std::tolower(*it);
    ++it;
  }
  return to_utf8(result);
}

std::string string_to_upper(const std::string& original)
{
  std::wstring result(from_utf8(original));
  auto it(result.begin());
  auto end(result.end());
  while (it != end) {
    *it = std::toupper(*it);
    ++it;
  }
  return to_utf8(result);
}

std::string to_utf8(const wchar_t* src)
{
  return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(src);
}

std::wstring from_utf8(const std::string& src)
{
  return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(src);
}

int utf8_length(const std::string& utf8string)
{
  utf8_const_iterator it(utf8string.begin());
  utf8_const_iterator end(utf8string.end());
  int c = 0;

  while (it != end)
    ++it, ++c;

  return c;
}

int utf8_icmp(const std::string& a, const std::string& b, int n)
{
  utf8_const_iterator a_it(a.begin());
  utf8_const_iterator a_end(a.end());
  utf8_const_iterator b_it(b.begin());
  utf8_const_iterator b_end(b.end());
  int i = 0;

  for (; (n == 0 || i < n) && a_it != a_end && b_it != b_end; ++a_it, ++b_it, ++i) {
    int a_chr = std::tolower(*a_it);
    int b_chr = std::tolower(*b_it);

    if (a_chr < b_chr)
      return -1;
    else if (a_chr > b_chr)
      return 1;
  }

  if (n > 0 && i == n)
    return 0;
  else if (a_it == a_end && b_it == b_end)
    return 0;
  else if (a_it == a_end)
    return -1;
  else
    return 1;
}

} // namespace base
