// LAF Base Library
// Copyright (c) 2026-present Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#if LAF_LINUX
  #define _FILE_OFFSET_BITS 64
  #define _LARGEFILE64_SOURCE
#endif

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/file_size.h"

#include "base/string.h"

#ifdef LAF_WINDOWS
  #include <io.h>
#else
  #include <fcntl.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#include <sys/stat.h>

namespace base {

fileoff_t base_ftell(FILE* file)
{
#if LAF_WINDOWS
  return _ftelli64(file);
#else
  return ftello(file);
#endif
}

int base_fseek(FILE* file, const fileoff_t offset, const int origin)
{
#if LAF_WINDOWS
  return _fseeki64(file, offset, origin);
#else
  return fseeko(file, offset, origin);
#endif
}

fileoff_t base_lseek(int fd, const fileoff_t offset, const int origin)
{
#if LAF_WINDOWS
  return _lseeki64(fd, offset, origin);
#elif LAF_MACOS
  return lseek(fd, offset, origin);
#else
  return lseek64(fd, offset, origin);
#endif
}

filesize_t file_size(FILE* file)
{
  const auto curpos = base_ftell(file);
  base_fseek(file, 0, SEEK_END);
  const auto size = base_ftell(file);
  base_fseek(file, curpos, SEEK_SET); // Restore old position
  return size;
}

filesize_t file_size(const std::string& path)
{
#if LAF_WINDOWS
  struct _stat64 sts;
  return (_wstat64(from_utf8(path).c_str(), &sts) == 0) ? sts.st_size : 0;
#else
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0) ? sts.st_size : 0;
#endif
}

} // namespace base
