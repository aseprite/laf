// LAF Base Library
// Copyright (c) 2021-2025 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/fs.h"
#include "base/split_string.h"
#include "base/string.h"
#include "base/utf8_decode.h"

#if LAF_WINDOWS
  #include "base/fs_win32.h"
#else
  #include "base/fs_unix.h"
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iterator>

namespace base {

// On Windows we can use \ or / as path separators, but on Unix-like
// platforms it's just /, as \ can be part of the file name.
#if LAF_WINDOWS
const std::string::value_type* path_separators = "\\/";
#else
const std::string::value_type* path_separators = "/";
#endif

bool is_absolute_path(const std::string& path)
{
#if LAF_WINDOWS
  return (path.size() > 2 && std::isalpha(path[0]) && path[1] == ':' &&
          path[2] == path_separator) ||
         // UNC network path
         (path.size() > 1 && path[0] == path_separator && path[1] == path_separator);
#else
  return !path.empty() && path[0] == path_separator;
#endif
}

void make_all_directories(const std::string& path)
{
  std::vector<std::string> parts;
  split_string(path, parts, path_separators);

  std::string intermediate;
  for (const std::string& component : parts) {
    if (component.empty()) {
      if (intermediate.empty())
        intermediate += "/";
      continue;
    }

    intermediate = join_path(intermediate, component);

    if (is_file(intermediate))
      throw std::runtime_error("Error creating directory (a component is a file name)");
    if (!is_directory(intermediate))
      make_directory(intermediate);
  }
}

std::string get_file_path(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string res;

  for (rit = filename.rbegin(); rit != filename.rend(); ++rit)
    if (is_path_separator(*rit))
      break;

  if (rit != filename.rend()) {
    ++rit;
    std::copy(filename.begin(), std::string::const_iterator(rit.base()), std::back_inserter(res));
  }

  return res;
}

std::string get_file_name(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string result;

  for (rit = filename.rbegin(); rit != filename.rend(); ++rit)
    if (is_path_separator(*rit))
      break;

  std::copy(std::string::const_iterator(rit.base()), filename.end(), std::back_inserter(result));

  return result;
}

std::string get_file_extension(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string result;

  // search for the first dot from the end of the string
  for (rit = filename.rbegin(); rit != filename.rend(); ++rit) {
    if (is_path_separator(*rit))
      return result;
    if (*rit == '.')
      break;
  }

  if (rit != filename.rend()) {
    std::copy(std::string::const_iterator(rit.base()), filename.end(), std::back_inserter(result));
  }

  return result;
}

std::string replace_extension(const std::string& filename, const std::string& extension)
{
  std::string::const_reverse_iterator rit;
  std::string result;

  // Search for the first dot from the end of the string.
  for (rit = filename.rbegin(); rit != filename.rend(); ++rit) {
    // Here is the dot of the extension.
    if (*rit == '.')
      break;
    // A path separator before a dot, i.e. the filename doesn't have a
    // extension.
    if (is_path_separator(*rit)) {
      rit = filename.rend();
      break;
    }
  }

  if (rit != filename.rend()) {
    auto it = std::string::const_iterator(rit.base());
    --it;
    std::copy(filename.begin(), it, std::back_inserter(result));
  }
  else {
    result = filename;
  }

  if (!extension.empty()) {
    result.push_back('.');
    result += extension;
  }

  return result;
}

std::string get_file_title(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string::const_iterator last_dot = filename.end();
  std::string result;

  for (rit = filename.rbegin(); rit != filename.rend(); ++rit) {
    if (is_path_separator(*rit))
      break;
    if (*rit == '.' && last_dot == filename.end())
      last_dot = rit.base() - 1;
  }

  for (std::string::const_iterator it(rit.base()); it != filename.end(); ++it) {
    if (it == last_dot)
      break;
    result.push_back(*it);
  }

  return result;
}

std::string get_file_title_with_path(const std::string& filename)
{
  std::string::const_reverse_iterator rit;

  // search for the first dot from the end of the string
  for (rit = filename.rbegin(); rit != filename.rend(); ++rit) {
    if (is_path_separator(*rit))
      return filename;
    if (*rit == '.')
      break;
  }

  if (rit != filename.rend())
    return filename.substr(0, rit.base() - filename.begin() - 1);
  return filename;
}

std::string get_relative_path(const std::string& filename, const std::string& base_path)
{
  std::vector<std::string> baseDirs;
  split_string(base_path, baseDirs, path_separators);

  std::vector<std::string> toParts;
  split_string(filename, toParts, path_separators);

  // Find the common prefix
  auto itFrom = baseDirs.begin();
  auto itTo = toParts.begin();

  while (itFrom != baseDirs.end() &&
         itTo != toParts.end()
#if LAF_LINUX
         // The Linux file system is case sensitive
         && *itFrom == *itTo
#else
         && base::string_to_lower(*itFrom) == base::string_to_lower(*itTo)
#endif
  ) {
    ++itFrom;
    ++itTo;
  }

  if (itFrom == baseDirs.begin() && itTo == toParts.begin()) {
    // No common prefix
    return filename;
  }

  // Calculate the number of directories to go up from base path
  std::string relativePath;
  for (auto it = itFrom; it != baseDirs.end(); ++it)
    relativePath = base::join_path(relativePath, "..");

  // Append the remaining part of 'toPath'
  for (auto it = itTo; it != toParts.end(); ++it)
    relativePath = base::join_path(relativePath, *it);

  return relativePath;
}

std::string join_path(const std::string& path, const std::string& file)
{
  std::string result(path);

  // Add a separator at the end if it is necessay
  if (!result.empty() && !is_path_separator(*(result.end() - 1)))
    result.push_back(path_separator);

  // Add the file
  result += file;
  return result;
}

std::string remove_path_separator(const std::string& path)
{
  std::string result(path);

  // Erase all trailing separators
  while (!result.empty() && is_path_separator(*(result.end() - 1)))
    result.erase(result.end() - 1);

  return result;
}

std::string fix_path_separators(const std::string& filename)
{
  std::string result;
  result.reserve(filename.size());

  size_t i = 0;

#if LAF_WINDOWS
  // Network paths can start with two backslashes
  if (filename.size() >= 2 && filename[0] == path_separator && // Check for equality to backslash
                                                               // (\),
      filename[1] == path_separator) {                         // no for is_path_separator()
    result.push_back(path_separator);
    result.push_back(path_separator);
    i += 2;
  }
#endif

  for (; i < filename.size(); ++i) {
    const auto chr = filename[i];
    if (is_path_separator(chr)) {
      if (result.empty() || !is_path_separator(result.back()))
        result.push_back(path_separator);
    }
    else
      result.push_back(chr);
  }
  return result;
}

// It tries to replicate the standard path::lexically_normal()
// algorithm from https://en.cppreference.com/w/cpp/filesystem/path
std::string normalize_path(const std::string& _path)
{
  // Normal form of an empty path is an empty path.
  if (_path.empty())
    return std::string();

  // Replace multiple slashes with a single path_separator.
  std::string path = fix_path_separators(_path);

  std::vector<std::string> parts;
  split_string(path, parts, path_separators);

  std::vector<std::string> fn_parts;
  const bool has_root = (!path.empty() && path[0] == path_separator);
  const bool last_dot = (parts.back() == ".");
  const bool last_slash = parts.back().empty();
  if (last_slash || last_dot)
    parts.pop_back();

  for (const auto& part : parts) {
    // Skip each dot part.
    if (part.empty() || part == ".")
      continue;
    if (part == "..") {
      if (has_root && fn_parts.empty())
        continue;
      if (!fn_parts.empty() && fn_parts.back() != "..")
        fn_parts.pop_back();
      else
        fn_parts.push_back(part);
    }
    else
      fn_parts.push_back(part);
  }

  // Reconstruct the filename 'fn' from 'fn_parts'
  std::string fn;
  fn.reserve(path.size());

  if (has_root) {
    fn.push_back(path_separator);
#if LAF_WINDOWS
    // Add the second separator for network paths.
    if (path.size() >= 2 && path[1] == path_separator) {
      fn.push_back(path_separator);
    }
#endif
  }

  for (const auto& part : fn_parts)
    fn = join_path(fn, part);

  if (!fn.empty() && parts.back() != ".." && (last_slash || last_dot))
    fn.push_back(path_separator);

  return (fn.empty() ? "." : fn);
}

bool has_file_extension(const std::string& filename, const base::paths& extensions)
{
  if (!filename.empty()) {
    const std::string ext = get_file_extension(filename);
    for (const auto& e : extensions)
      if (utf8_icmp(ext, e) == 0)
        return true;
  }
  return false;
}

int compare_filenames(const std::string& a, const std::string& b)
{
  utf8_decode a_dec(a), b_dec(b);

  while (!a_dec.is_end() && !b_dec.is_end()) {
    int a_chr = a_dec.next();
    if (!a_chr)
      break;

    int b_chr = b_dec.next();
    if (!b_chr)
      break;

    if ((a_chr >= '0') && (a_chr <= '9') && (b_chr >= '0') && (b_chr <= '9')) {
      auto a_dec2 = a_dec;
      auto b_dec2 = b_dec;

      int a_num = (a_chr - '0');
      while (const int c = a_dec2.next()) {
        if ((c >= '0') && (c <= '9'))
          a_num = (a_num * 10 + (c - '0'));
        else
          break;
      }

      int b_num = (b_chr - '0');
      while (const int c = b_dec2.next()) {
        if ((c >= '0') && (c <= '9'))
          b_num = (b_num * 10 + (c - '0'));
        else
          break;
      }

      if (a_num != b_num)
        return a_num - b_num < 0 ? -1 : 1;
    }
    else if (is_path_separator(a_chr) && is_path_separator(b_chr)) {
      // Go to next char
    }
    else {
      a_chr = std::tolower(a_chr);
      b_chr = std::tolower(b_chr);

      if (a_chr != b_chr)
        return a_chr - b_chr < 0 ? -1 : 1;
    }
  }

  if (a_dec.is_end() && b_dec.is_end())
    return 0;
  if (a_dec.is_end())
    return -1;
  return 1;
}

} // namespace base
