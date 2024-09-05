// LAF Base Library
// Copyright (c) 2020-2024 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/fs.h"
#include "base/paths.h"
#include "base/string.h"
#include "base/time.h"
#include "base/version.h"
#include "base/win/win32_exception.h"

#include <stdexcept>
#include <windows.h>
#include <shlobj.h>
#include <sys/stat.h>

namespace base {

bool is_file(const std::string& path)
{
  DWORD attr = ::GetFileAttributes(from_utf8(path).c_str());

  // GetFileAttributes returns INVALID_FILE_ATTRIBUTES in case of
  // fail.
  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool is_directory(const std::string& path)
{
  DWORD attr = ::GetFileAttributes(from_utf8(path).c_str());

  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
}

size_t file_size(const std::string& path)
{
  struct _stat sts;
  return (_wstat(from_utf8(path).c_str(), &sts) == 0) ? sts.st_size: 0;
}

void move_file(const std::string& src, const std::string& dst)
{
  BOOL result = ::MoveFile(from_utf8(src).c_str(), from_utf8(dst).c_str());
  if (result == 0)
    throw Win32Exception("Error moving file");
}

void copy_file(const std::string& src, const std::string& dst, bool overwrite)
{
  BOOL result = ::CopyFile(from_utf8(src).c_str(), from_utf8(dst).c_str(), !overwrite);
  if (result == 0)
    throw Win32Exception("Error copying file");
}

void delete_file(const std::string& path)
{
  BOOL result = ::DeleteFile(from_utf8(path).c_str());
  if (result == 0)
    throw Win32Exception("Error deleting file");
}

bool has_readonly_attr(const std::string& path)
{
  DWORD attr = ::GetFileAttributes(from_utf8(path).c_str());

  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          ((attr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY));
}

void remove_readonly_attr(const std::string& path)
{
  std::wstring fn = from_utf8(path);
  DWORD attr = ::GetFileAttributes(fn.c_str());

  if ((attr != INVALID_FILE_ATTRIBUTES) &&
      (attr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY) {
    ::SetFileAttributes(fn.c_str(), attr & ~FILE_ATTRIBUTE_READONLY);
  }
}

Time get_modification_time(const std::string& path)
{
  WIN32_FILE_ATTRIBUTE_DATA data;
  ZeroMemory(&data, sizeof(data));

  std::wstring fn = from_utf8(path);
  if (!GetFileAttributesEx(fn.c_str(), GetFileExInfoStandard, (LPVOID)&data))
    return Time();

  SYSTEMTIME utc, local;
  FileTimeToSystemTime(&data.ftLastWriteTime, &utc);
  SystemTimeToTzSpecificLocalTime(NULL, &utc, &local);

  return Time(
    local.wYear, local.wMonth, local.wDay,
    local.wHour, local.wMinute, local.wSecond);
}

void make_directory(const std::string& path)
{
  BOOL result = ::CreateDirectory(from_utf8(path).c_str(), NULL);
  if (result == 0)
    throw Win32Exception("Error creating directory");
}

void remove_directory(const std::string& path)
{
  BOOL result = ::RemoveDirectory(from_utf8(path).c_str());
  if (result == 0)
    throw Win32Exception("Error removing directory");
}

std::string get_current_path()
{
  TCHAR buffer[MAX_PATH+1];
  if (::GetCurrentDirectory(sizeof(buffer)/sizeof(TCHAR), buffer))
    return to_utf8(buffer);
  return std::string();
}

void set_current_path(const std::string& path)
{
  ::SetCurrentDirectory(from_utf8(path).c_str());
}

std::string get_app_path()
{
  TCHAR buffer[MAX_PATH+1];
  if (::GetModuleFileName(NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
    return to_utf8(buffer);
  return std::string();
}

std::string get_temp_path()
{
  TCHAR buffer[MAX_PATH+1];
  DWORD result = ::GetTempPath(sizeof(buffer)/sizeof(TCHAR), buffer);
  return to_utf8(buffer);
}

std::string get_user_docs_folder()
{
  TCHAR buffer[MAX_PATH+1];
  HRESULT hr = SHGetFolderPath(
    NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT,
    buffer);
  if (hr == S_OK)
    return to_utf8(buffer);
  return std::string();
}

std::string get_canonical_path(const std::string& path)
{
  std::string full = get_absolute_path(path);
  DWORD attr = ::GetFileAttributes(from_utf8(full).c_str());
  if (attr != INVALID_FILE_ATTRIBUTES)
    return full;
  return std::string();
}

std::string get_absolute_path(const std::string& path)
{
  std::string full;
  if (path.size() > 2 && path[1] != ':')
    full = base::join_path(base::get_current_path(), path);
  else
    full = path;

  TCHAR buffer[MAX_PATH+1];
  GetFullPathName(
    from_utf8(full).c_str(),
    sizeof(buffer)/sizeof(TCHAR),
    buffer,
    nullptr);
  return to_utf8(buffer);
}

paths list_files(const std::string& path,
                       ItemType filter,
                       const std::string& match)
{
  WIN32_FIND_DATA fd;
  paths files;
  HANDLE handle = FindFirstFileEx(
    base::from_utf8(base::join_path(path, match)).c_str(),
    FindExInfoBasic,
    &fd,
    (filter == ItemType::Directories) ? FindExSearchLimitToDirectories :
                                             FindExSearchNameMatch,
    NULL,
    0);

  if (handle == INVALID_HANDLE_VALUE)
    return files;

  do {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (filter == ItemType::Files)
        continue;

      if (lstrcmpW(fd.cFileName, L".") == 0 ||
          lstrcmpW(fd.cFileName, L"..") == 0)
        continue;
    }
    else if (filter == ItemType::Directories)
      continue;

    files.push_back(base::to_utf8(fd.cFileName));
  } while (FindNextFile(handle, &fd));

  FindClose(handle);
  return files;
}

Version get_file_version(const std::string& filename)
{
  return get_file_version(from_utf8(filename).c_str());
}

Version get_file_version(const wchar_t* filename)
{
  DWORD handle; // From MS docs, this is set to zero by GetFileVersionInfoSizeW()
  DWORD size = GetFileVersionInfoSizeW(filename, &handle);
  if (size == 0)
    return Version();

  // The second param of GetFileVersionInfoW() is ignored, so we pass just 0
  std::vector<uint8_t> data(size);
  if (!GetFileVersionInfoW(filename, 0, size, &data[0]))
    return Version();

  VS_FIXEDFILEINFO* fi = nullptr;
  UINT fiLen = 0;
  if (!VerQueryValueW(&data[0], L"\\", (LPVOID*)&fi, &fiLen) ||
      fiLen < sizeof(VS_FIXEDFILEINFO) ||
      fi->dwSignature != 0xfeef04bd) {
    return Version();
  }

  return Version(fi->dwFileVersionMS >> 16, fi->dwFileVersionMS & 0xffff,
                 fi->dwFileVersionLS >> 16, fi->dwFileVersionLS & 0xffff);
}

}
