// LAF Base Library
// Copyright (c) 2020-2021 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_FS_H_INCLUDED
#define BASE_FS_H_INCLUDED
#pragma once

#include <string>

#include "base/paths.h"

namespace base {

  class Time;

  // Default path separator (on Windows it is '\' and on Unix-like systems it is '/').
  extern const std::string::value_type path_separator;

  bool is_file(const std::string& path);
  bool is_directory(const std::string& path);

  size_t file_size(const std::string& path);

  bool move_file(const std::string& src, const std::string& dst);
  bool copy_file(const std::string& src, const std::string& dst, bool overwrite);
  // TODO: Change bool to enum Result?
  bool delete_file(const std::string& path);

  bool has_readonly_attr(const std::string& path);
  void remove_readonly_attr(const std::string& path);

  Time get_modification_time(const std::string& path);

  void make_directory(const std::string& path);
  void make_all_directories(const std::string& path);
  void remove_directory(const std::string& path);

  std::string get_current_path();
  std::string get_app_path();
  std::string get_temp_path();
  std::string get_user_docs_folder();
#if __APPLE__
  std::string get_lib_app_support_path();
#endif

  // If the given filename is a relative path, it converts the
  // filename to an absolute one.
  std::string get_canonical_path(const std::string& path);

  // TODO why get_canonical_path() is not enough?
  std::string get_absolute_path(const std::string& filename);

  paths list_files(const std::string& path);

  // Returns true if the given character is a valud path separator
  // (any of '\' or '/' characters).
  bool is_path_separator(std::string::value_type chr);

  // Returns only the path (without the last trailing slash).
  std::string get_file_path(const std::string& filename);

  // Returns the file name with its extension, removing the path.
  std::string get_file_name(const std::string& filename);

  // Returns the extension of the file name (without the dot).
  std::string get_file_extension(const std::string& filename);

  // Returns the whole path with another extension.
  std::string replace_extension(const std::string& filename, const std::string& extension);

  // Returns the file name without path and without extension.
  std::string get_file_title(const std::string& filename);
  std::string get_file_title_with_path(const std::string& filename);

  // Joins two paths or a path and a file name with a path-separator.
  std::string join_path(const std::string& path, const std::string& file);

  // Removes the trailing separator from the given path.
  std::string remove_path_separator(const std::string& path);

  // Replaces all separators with the system separator.
  std::string fix_path_separators(const std::string& filename);

  // Calls get_canonical_path() and fix_path_separators() for the
  // given filename.
  std::string normalize_path(const std::string& filename);

  // Returns true if the filename contains one of the specified
  // extensions. The "extensions" parameter must be a set of possible
  // extensions.
  bool has_file_extension(const std::string& filename, const base::paths& extensions);

  int compare_filenames(const std::string& a, const std::string& b);

#if LAF_WINDOWS
  class Version;
  Version get_file_version(const std::string& filename);
  Version get_file_version(const wchar_t* filename);
#endif

} // namespace base

#endif
