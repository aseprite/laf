// LAF Base Library
// Copyright (c) 2020-2024 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/debug.h"
#include "base/string.h"
#include "base/utf8_decode.h"

#include <cctype>
#include <vector>

#ifdef LAF_WINDOWS
  #include <windows.h>
#endif

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
  const size_t required_size = std::vsnprintf(nullptr, 0, format, ap);
  if (required_size > 0) {
    buf.resize(required_size+1);
    std::vsnprintf(buf.data(), buf.size(), format, ap2);
  }
  va_end(ap2);
  return std::string(buf.data());
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

std::string removeEscapeCharFromText(const std::string& original,
                                     const int escapeChar)
{
  std::wstring newText; // wstring is used to properly push_back() multibyte chars
  newText.reserve(original.size());

  utf8_decode decode(original);
  while (int chr = decode.next()) {
    if (chr == escapeChar) {
      chr = decode.next();
      if (!chr)
        break;    // Ill-formed string (it ends with escape character)
    }
    newText.push_back(chr);
  }
  return to_utf8(newText);
}

#ifdef LAF_WINDOWS

std::string to_utf8(const wchar_t* src, const size_t n)
{
  int required_size =
    ::WideCharToMultiByte(CP_UTF8, 0,
      src, (int)n,
      NULL, 0, NULL, NULL);

  if (required_size == 0)
    return std::string();

  std::vector<char> buf(++required_size);

  ::WideCharToMultiByte(CP_UTF8, 0,
    src, (int)n,
    &buf[0], required_size,
    NULL, NULL);

  return std::string(&buf[0]);
}

std::wstring from_utf8(const std::string& src)
{
  int required_size =
    MultiByteToWideChar(CP_UTF8, 0,
      src.c_str(), (int)src.size(),
      NULL, 0);

  if (required_size == 0)
    return std::wstring();

  std::vector<wchar_t> buf(++required_size);

  ::MultiByteToWideChar(CP_UTF8, 0,
    src.c_str(), (int)src.size(),
    &buf[0], required_size);

  return std::wstring(&buf[0]);
}

#else

// Based on Allegro Unicode code (allegro/src/unicode.c)
static std::size_t insert_utf8_char(std::string* result, wchar_t chr)
{
  int size, bits, b, i;

  if (chr < 128) {
    if (result)
      result->push_back(chr);
    return 1;
  }

  bits = 7;
  while (chr >= (1<<bits))
    bits++;

  size = 2;
  b = 11;

  while (b < bits) {
    size++;
    b += 5;
  }

  if (result) {
    b -= (7-size);
    int firstbyte = chr>>b;
    for (i=0; i<size; i++)
      firstbyte |= (0x80>>i);

    result->push_back(firstbyte);

    for (i=1; i<size; i++) {
      b -= 6;
      result->push_back(0x80 | ((chr>>b)&0x3F));
    }
  }

  return size;
}

std::string to_utf8(const wchar_t* src, const size_t n)
{
  // Get required size to reserve a string so string::push_back()
  // doesn't need to reallocate its data.
  std::size_t required_size = 0;
  const auto* p = src;
  for (size_t i=0; i<n; ++i, ++p)
    required_size += insert_utf8_char(nullptr, *p);
  if (!required_size)
    return "";

  std::string result;
  result.reserve(++required_size);
  p = src;
  for (int i=0; i<n; ++i, ++p)
    insert_utf8_char(&result, *p);
  return result;
}

std::wstring from_utf8(const std::string& src)
{
  int required_size = utf8_length(src);
  std::vector<wchar_t> buf(++required_size);
  std::vector<wchar_t>::iterator buf_it = buf.begin();
#ifdef _DEBUG
  std::vector<wchar_t>::iterator buf_end = buf.end();
#endif
  utf8_decode decode(src);

  while (const int chr = decode.next()) {
    ASSERT(buf_it != buf_end);
    *buf_it = chr;
    ++buf_it;
  }

  return std::wstring(buf.data());
}

#endif

int utf8_length(const std::string& utf8string)
{
  utf8_decode decode(utf8string);
  int c = 0;

  while (decode.next())
    ++c;

  return c;
}

int utf8_icmp(const std::string& a, const std::string& b, int n)
{
  utf8_decode a_decode(a);
  utf8_decode b_decode(b);
  int i = 0;

  for (; (n == 0 || i < n)
         && !a_decode.is_end()
         && !b_decode.is_end(); ++i) {
    int a_chr = a_decode.next();
    if (!a_chr)
      break;

    int b_chr = b_decode.next();
    if (!b_chr)
      break;

    a_chr = std::tolower(a_chr);
    b_chr = std::tolower(b_chr);

    if (a_chr < b_chr) return -1;
    if (a_chr > b_chr) return 1;
  }

  if (n > 0 && i == n)
    return 0;
  if (a_decode.is_end() && b_decode.is_end())
    return 0;
  if (a_decode.is_end())
    return -1;
  return 1;
}

} // namespace base
