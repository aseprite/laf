// LAF Base Library
// Copyright (c) 2021 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <climits>              // Required for PATH_MAX
#include <cstdio>               // Required for rename()
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <vector>

#if __APPLE__
  #include <mach-o/dyld.h>
#elif __FreeBSD__
  #include <sys/sysctl.h>
#endif

#include "base/paths.h"
#include "base/time.h"

#define MAXPATHLEN 1024

namespace base {

bool is_file(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && S_ISREG(sts.st_mode)) ? true: false;
}

bool is_directory(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && S_ISDIR(sts.st_mode)) ? true: false;
}

void make_directory(const std::string& path)
{
  int result = mkdir(path.c_str(), 0777);
  if (result != 0) {
    throw std::runtime_error("Error creating directory: " +
                             std::string(std::strerror(errno)));
  }
}

size_t file_size(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0) ? sts.st_size: 0;
}

void move_file(const std::string& src, const std::string& dst)
{
  int result = std::rename(src.c_str(), dst.c_str());
  if (result != 0)
    throw std::runtime_error("Error moving file: " +
                             std::string(std::strerror(errno)));
}

void copy_file(const std::string& src, const std::string& dst, bool overwrite)
{
  throw std::runtime_error("Error copying file: unimplemented");
}

void delete_file(const std::string& path)
{
  int result = unlink(path.c_str());
  if (result != 0)
    throw std::runtime_error("Error deleting file: " +
                             std::string(std::strerror(errno)));
}

bool has_readonly_attr(const std::string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && ((sts.st_mode & S_IWUSR) == 0));
}

void remove_readonly_attr(const std::string& path)
{
  struct stat sts;
  int result = stat(path.c_str(), &sts);
  if (result == 0) {
    result = chmod(path.c_str(), sts.st_mode | S_IWUSR);
    if (result != 0)
      throw std::runtime_error("Error removing read-only attribute: " +
                               std::string(std::strerror(errno)));
  }
}

Time get_modification_time(const std::string& path)
{
  struct stat sts;
  int result = stat(path.c_str(), &sts);
  if (result != 0)
    return Time();

  std::tm t;
  safe_localtime(sts.st_mtime, &t);
  return Time(
    t.tm_year+1900, t.tm_mon+1, t.tm_mday,
    t.tm_hour, t.tm_min, t.tm_sec);
}

void remove_directory(const std::string& path)
{
  int result = rmdir(path.c_str());
  if (result != 0)
    throw std::runtime_error("Error removing directory: " +
                             std::string(std::strerror(errno)));
}

std::string get_current_path()
{
  std::vector<char> path(MAXPATHLEN);
  if (getcwd(&path[0], path.size()))
    return std::string(&path[0]);
  return std::string();
}

std::string get_app_path()
{
  std::vector<char> path(MAXPATHLEN);

#if __APPLE__
  uint32_t size = path.size();
  while (_NSGetExecutablePath(&path[0], &size) == -1)
    path.resize(size);
#elif __FreeBSD__
  size_t size = path.size();
  const int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
  while (sysctl(mib, 4, &path[0], &size, NULL, 0) == -1)
      path.resize(size);
#else  // Linux
  if (readlink("/proc/self/exe", &path[0], path.size()) == -1)
    return std::string();
#endif

  return std::string(&path[0]);
}

std::string get_temp_path()
{
  char* tmpdir = getenv("TMPDIR");
  if (tmpdir)
    return tmpdir;
  return "/tmp";
}

std::string get_user_docs_folder()
{
  char* tmpdir = getenv("HOME");
  if (tmpdir)
    return tmpdir;
  return "/";
}

std::string get_canonical_path(const std::string& path)
{
  char buffer[PATH_MAX];
  // Ignore return value as realpath() returns nullptr anyway when the
  // resolved_path parameter is specified.
  realpath(path.c_str(), buffer);
  return buffer;
}

paths list_files(const std::string& path)
{
  paths files;
  DIR* handle = opendir(path.c_str());
  if (handle) {
    dirent* item;
    while ((item = readdir(handle)) != nullptr) {
      std::string filename = item->d_name;
      if (filename != "." && filename != "..")
        files.push_back(filename);
    }

    closedir(handle);
  }
  return files;
}

}
