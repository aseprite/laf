// LAF Base Library
// Copyright (c) 2020-2024 Igara Studio S.A.
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

// Default path separator (on Windows it is '\' and on Unix-like
// systems it is '/').
#if LAF_WINDOWS
static constexpr const std::string::value_type path_separator = '\\';
#else
static constexpr const std::string::value_type path_separator = '/';
#endif
extern const std::string::value_type* path_separators;

bool is_file(const std::string& path);
bool is_directory(const std::string& path);

size_t file_size(const std::string& path);

void move_file(const std::string& src, const std::string& dst);
void copy_file(const std::string& src, const std::string& dst, bool overwrite);
void delete_file(const std::string& path);

bool has_readonly_attr(const std::string& path);
void remove_readonly_attr(const std::string& path);

Time get_modification_time(const std::string& path);

void make_directory(const std::string& path);
void make_all_directories(const std::string& path);
void remove_directory(const std::string& path);

std::string get_current_path();
void set_current_path(const std::string& path);

std::string get_app_path();
std::string get_temp_path();
std::string get_user_docs_folder();
#if __APPLE__
std::string get_lib_app_support_path();
#endif

// Converts an existing file path to an absolute one, or returns an
// empty string if the file doesn't exist. It uses realpath() on
// POSIX-like systems and GetFullPathName() on Windows.
std::string get_canonical_path(const std::string& path);

// Returns the absolute path using lexical/string operations, and
// get_current_path() when needed. Doesn't require an existing file
// in "path". The returned path shouldn't contain "." or ".."
// elements (is a normalized path).
std::string get_absolute_path(const std::string& path);

// Item types that list_files() can be filtered by
enum class ItemType { All, Directories, Files };

// List all the items in a given path by default, two other parameters second parameters:
// filter can be use to distinguish between All items, directories and files.
// The name search can be used to match files by extension with something like "*.png" or by exact
// match without wildcards.
paths list_files(const std::string& path,
                 ItemType filter = ItemType::All,
                 const std::string& = "*");

// Returns true if the given character is a valud path separator
// (any of '\' or '/' characters).
inline constexpr bool is_path_separator(std::string::value_type chr)
{
  return (
#if LAF_WINDOWS
    chr == '\\' ||
#endif
    chr == '/');
}

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

  // Returns the relative path of the given filename from the base_path.
  std::string get_relative_path(const std::string& filename, const std::string& base_path);

  // Returns the absolute path of a given relative filename referred to the base_path.
  std::string get_absolute_path_from_relative(const std::string& base_path,
                                              const std::string& relative_filename);

  // Joins two paths or a path and a file name with a path-separator.
  std::string join_path(const std::string& path, const std::string& file);

  // Removes the trailing separator from the given path.
  std::string remove_path_separator(const std::string& path);

  // Replaces all separators with the system separator.
  std::string fix_path_separators(const std::string& filename);

  // Remove superfluous path elements ("/../" and "/./") and call
  // fix_path_separators() for the given path.
  std::string normalize_path(const std::string& path);

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
