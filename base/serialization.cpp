// LAF Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/mem_utils.h"
#include "base/serialization.h"

#include <iostream>

namespace base {
namespace serialization {

std::ostream& write8(std::ostream& os, uint8_t byte)
{
  os.put(byte);
  return os;
}

uint8_t read8(std::istream& is)
{
  return (uint8_t)is.get();
}

std::ostream& little_endian::write16(std::ostream& os, uint16_t word)
{
  os.put((int)((word & 0x00ff)));
  os.put((int)((word & 0xff00) >> 8));
  return os;
}

std::ostream& little_endian::write32(std::ostream& os, uint32_t dword)
{
  os.put((int)((dword & 0x000000ffl)));
  os.put((int)((dword & 0x0000ff00l) >> 8));
  os.put((int)((dword & 0x00ff0000l) >> 16));
  os.put((int)((dword & 0xff000000l) >> 24));
  return os;
}

std::ostream& little_endian::write64(std::ostream& os, uint64_t qword)
{
  os.put((int)((qword & 0x00000000000000ffl)));
  os.put((int)((qword & 0x000000000000ff00l) >> 8));
  os.put((int)((qword & 0x0000000000ff0000l) >> 16));
  os.put((int)((qword & 0x00000000ff000000l) >> 24));
  os.put((int)((qword & 0x000000ff00000000l) >> 32));
  os.put((int)((qword & 0x0000ff0000000000l) >> 40));
  os.put((int)((qword & 0x00ff000000000000l) >> 48));
  os.put((int)((qword & 0xff00000000000000l) >> 56));
  return os;
}

std::ostream& little_endian::write_float(std::ostream& os, float value)
{
  const int b = *(copy_reinterpret_cast<int*>(&value));
  os.put((int)((b & 0x000000ffl)));
  os.put((int)((b & 0x0000ff00l) >> 8));
  os.put((int)((b & 0x00ff0000l) >> 16));
  os.put((int)((b & 0xff000000l) >> 24));
  return os;
}

std::ostream& little_endian::write_double(std::ostream& os, double value)
{
  const long long b = *(copy_reinterpret_cast<long long*>(&value));
  os.put((int)((b & 0x00000000000000ffl)));
  os.put((int)((b & 0x000000000000ff00l) >> 8));
  os.put((int)((b & 0x0000000000ff0000l) >> 16));
  os.put((int)((b & 0x00000000ff000000l) >> 24));
  os.put((int)((b & 0x000000ff00000000l) >> 32));
  os.put((int)((b & 0x0000ff0000000000l) >> 40));
  os.put((int)((b & 0x00ff000000000000l) >> 48));
  os.put((int)((b & 0xff00000000000000l) >> 56));
  return os;
}

uint16_t little_endian::read16(std::istream& is)
{
  int b1, b2;
  b1 = is.get();
  b2 = is.get();
  return ((b2 << 8) | b1);
}

uint32_t little_endian::read32(std::istream& is)
{
  int b1, b2, b3, b4;
  b1 = is.get();
  b2 = is.get();
  b3 = is.get();
  b4 = is.get();
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

uint64_t little_endian::read64(std::istream& is)
{
  int b1, b2, b3, b4, b5, b6, b7, b8;
  b1 = is.get();
  b2 = is.get();
  b3 = is.get();
  b4 = is.get();
  b5 = is.get();
  b6 = is.get();
  b7 = is.get();
  b8 = is.get();
  return (((long long)b8 << 56) |
          ((long long)b7 << 48) |
          ((long long)b6 << 40) |
          ((long long)b5 << 32) |
          ((long long)b4 << 24) |
          ((long long)b3 << 16) |
          ((long long)b2 << 8) |
          (long long)b1);
}

float little_endian::read_float(std::istream& is)
{
  int b1, b2, b3, b4;
  b1 = is.get();
  b2 = is.get();
  b3 = is.get();
  b4 = is.get();
  int v = ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
  return *copy_reinterpret_cast<float*>(&v);
}

double little_endian::read_double(std::istream& is)
{
  int b1, b2, b3, b4, b5, b6, b7, b8;
  b1 = is.get();
  b2 = is.get();
  b3 = is.get();
  b4 = is.get();
  b5 = is.get();
  b6 = is.get();
  b7 = is.get();
  b8 = is.get();
  int v = (((long long)b8 << 56) |
           ((long long)b7 << 48) |
           ((long long)b6 << 40) |
           ((long long)b5 << 32) |
           ((long long)b4 << 24) |
           ((long long)b3 << 16) |
           ((long long)b2 << 8) |
           (long long)b1);
  return *copy_reinterpret_cast<double*>(&v);
}

std::ostream& big_endian::write16(std::ostream& os, uint16_t word)
{
  os.put((int)((word & 0xff00) >> 8));
  os.put((int)((word & 0x00ff)));
  return os;
}

std::ostream& big_endian::write32(std::ostream& os, uint32_t dword)
{
  os.put((int)((dword & 0xff000000l) >> 24));
  os.put((int)((dword & 0x00ff0000l) >> 16));
  os.put((int)((dword & 0x0000ff00l) >> 8));
  os.put((int)((dword & 0x000000ffl)));
  return os;
}

std::ostream& big_endian::write64(std::ostream& os, uint64_t qword)
{
  os.put((int)((qword & 0xff00000000000000l) >> 56));
  os.put((int)((qword & 0x00ff000000000000l) >> 48));
  os.put((int)((qword & 0x0000ff0000000000l) >> 40));
  os.put((int)((qword & 0x000000ff00000000l) >> 32));
  os.put((int)((qword & 0x00000000ff000000l) >> 24));
  os.put((int)((qword & 0x0000000000ff0000l) >> 16));
  os.put((int)((qword & 0x000000000000ff00l) >> 8));
  os.put((int)((qword & 0x00000000000000ffl)));
  return os;
}

std::ostream& big_endian::write_float(std::ostream& os, float value)
{
  const int b = *(copy_reinterpret_cast<int*>(&value));
  os.put((int)((b & 0xff000000l) >> 24));
  os.put((int)((b & 0x00ff0000l) >> 16));
  os.put((int)((b & 0x0000ff00l) >> 8));
  os.put((int)((b & 0x000000ffl)));
  return os;
}

std::ostream& big_endian::write_double(std::ostream& os, double value)
{
  const long long b = *(copy_reinterpret_cast<long long*>(&value));
  os.put((int)((b & 0xff00000000000000l) >> 56));
  os.put((int)((b & 0x00ff000000000000l) >> 48));
  os.put((int)((b & 0x0000ff0000000000l) >> 40));
  os.put((int)((b & 0x000000ff00000000l) >> 32));
  os.put((int)((b & 0x00000000ff000000l) >> 24));
  os.put((int)((b & 0x0000000000ff0000l) >> 16));
  os.put((int)((b & 0x000000000000ff00l) >> 8));
  os.put((int)((b & 0x00000000000000ffl)));
  return os;
}

uint16_t big_endian::read16(std::istream& is)
{
  int b1, b2;
  b2 = is.get();
  b1 = is.get();
  return ((b2 << 8) | b1);
}

uint32_t big_endian::read32(std::istream& is)
{
  int b1, b2, b3, b4;
  b4 = is.get();
  b3 = is.get();
  b2 = is.get();
  b1 = is.get();
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

uint64_t big_endian::read64(std::istream& is)
{
  int b1, b2, b3, b4, b5, b6, b7, b8;
  b8 = is.get();
  b7 = is.get();
  b6 = is.get();
  b5 = is.get();
  b4 = is.get();
  b3 = is.get();
  b2 = is.get();
  b1 = is.get();
  return (((long long)b8 << 56) |
          ((long long)b7 << 48) |
          ((long long)b6 << 40) |
          ((long long)b5 << 32) |
          ((long long)b4 << 24) |
          ((long long)b3 << 16) |
          ((long long)b2 << 8) |
          (long long)b1);
}

float big_endian::read_float(std::istream& is)
{
  int b1, b2, b3, b4;
  b4 = is.get();
  b3 = is.get();
  b2 = is.get();
  b1 = is.get();
  int v = ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
  return *copy_reinterpret_cast<float*>(&v);
}

double big_endian::read_double(std::istream& is)
{
  int b1, b2, b3, b4, b5, b6, b7, b8;
  b8 = is.get();
  b7 = is.get();
  b6 = is.get();
  b5 = is.get();
  b4 = is.get();
  b3 = is.get();
  b2 = is.get();
  b1 = is.get();
  int v = (((long long)b8 << 56) |
           ((long long)b7 << 48) |
           ((long long)b6 << 40) |
           ((long long)b5 << 32) |
           ((long long)b4 << 24) |
           ((long long)b3 << 16) |
           ((long long)b2 << 8) |
           (long long)b1);
  return *copy_reinterpret_cast<double*>(&v);
}


} // namespace serialization
} // namespace base
