// LAF Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>

#include "mem_utils.h"

namespace base {

// Reads a WORD (16 bits) using little-endian byte ordering.
int fgetw(FILE* file)
{
  int b1, b2;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  // Little endian.
  return ((b2 << 8) | b1);
}

// Reads a DWORD (32 bits) using little-endian byte ordering.
long fgetl(FILE* file)
{
  int b1, b2, b3, b4;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  b3 = fgetc(file);
  if (b3 == EOF)
    return EOF;

  b4 = fgetc(file);
  if (b4 == EOF)
    return EOF;

  // Little endian.
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

// Reads a QWORD (64 bits) using little-endian byte ordering.
long long fgetq(FILE* file)
{
  int b1, b2, b3, b4, b5, b6, b7, b8;

  if ((b1 = fgetc(file)) == EOF)
    return EOF;

  if ((b2 = fgetc(file)) == EOF)
    return EOF;

  if ((b3 = fgetc(file)) == EOF)
    return EOF;

  if ((b4 = fgetc(file)) == EOF)
    return EOF;

  if ((b5 = fgetc(file)) == EOF)
    return EOF;

  if ((b6 = fgetc(file)) == EOF)
    return EOF;

  if ((b7 = fgetc(file)) == EOF)
    return EOF;

  if ((b8 = fgetc(file)) == EOF)
    return EOF;

  // Little endian.
  return (((long long)b8 << 56) |
          ((long long)b7 << 48) |
          ((long long)b6 << 40) |
          ((long long)b5 << 32) |
          ((long long)b4 << 24) |
          ((long long)b3 << 16) |
          ((long long)b2 << 8) |
          (long long)b1);
}

// Reads a 32-bit single-precision floating point number using
// little-endian byte ordering.
float fgetf(FILE* file)
{
  int b1, b2, b3, b4;

  if ((b1 = fgetc(file)) == EOF)
    return EOF;

  if ((b2 = fgetc(file)) == EOF)
    return EOF;

  if ((b3 = fgetc(file)) == EOF)
    return EOF;

  if ((b4 = fgetc(file)) == EOF)
    return EOF;

  // Little endian.
  int v = ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
  return *copy_reinterpret_cast<float*>(&v);
}

// Reads a 64-bit double-precision floating point number using
// little-endian byte ordering.
double fgetd(FILE* file)
{
  int b1, b2, b3, b4, b5, b6, b7, b8;

  if ((b1 = fgetc(file)) == EOF)
    return EOF;

  if ((b2 = fgetc(file)) == EOF)
    return EOF;

  if ((b3 = fgetc(file)) == EOF)
    return EOF;

  if ((b4 = fgetc(file)) == EOF)
    return EOF;

  if ((b5 = fgetc(file)) == EOF)
    return EOF;

  if ((b6 = fgetc(file)) == EOF)
    return EOF;

  if ((b7 = fgetc(file)) == EOF)
    return EOF;

  if ((b8 = fgetc(file)) == EOF)
    return EOF;

  // Little endian.
  long long v = (((long long)b8 << 56) |
                 ((long long)b7 << 48) |
                 ((long long)b6 << 40) |
                 ((long long)b5 << 32) |
                 ((long long)b4 << 24) |
                 ((long long)b3 << 16) |
                 ((long long)b2 << 8) |
                 (long long)b1);
  return *copy_reinterpret_cast<double*>(&v);
}

// Writes a word using little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputw(int w, FILE* file)
{
  int b1, b2;

  // Little endian.
  b2 = (w & 0xFF00) >> 8;
  b1 = w & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      return 0;

  return -1;
}

// Writes DWORD a using little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputl(long l, FILE* file)
{
  int b1, b2, b3, b4;

  // Little endian.
  b4 = (int)((l & 0xFF000000L) >> 24);
  b3 = (int)((l & 0x00FF0000L) >> 16);
  b2 = (int)((l & 0x0000FF00L) >> 8);
  b1 = (int)l & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      if (fputc(b3, file) == b3)
        if (fputc(b4, file) == b4)
          return 0;

  return -1;
}

// Writes a QWORD using little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputq(long long l, FILE* file)
{
  int b1, b2, b3, b4, b5, b6, b7, b8;

  // Little endian.
  b8 = (int)((l & 0xFF00000000000000L) >> 56);
  b7 = (int)((l & 0x00FF000000000000L) >> 48);
  b6 = (int)((l & 0x0000FF0000000000L) >> 40);
  b5 = (int)((l & 0x000000FF00000000L) >> 32);
  b4 = (int)((l & 0x00000000FF000000L) >> 24);
  b3 = (int)((l & 0x0000000000FF0000L) >> 16);
  b2 = (int)((l & 0x000000000000FF00L) >> 8);
  b1 = (int)l & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      if (fputc(b3, file) == b3)
        if (fputc(b4, file) == b4)
          if (fputc(b5, file) == b5)
            if (fputc(b6, file) == b6)
              if (fputc(b7, file) == b7)
                if (fputc(b8, file) == b8)
                  return 0;

  return -1;
}

// Writes a 32-bit single-precision floating point number using
// little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputf(float l, FILE* file)
{
  const int b = *(copy_reinterpret_cast<int*>(&l));
  int b1, b2, b3, b4;

  // Little endian.
  b4 = (int)((b & 0xFF000000L) >> 24);
  b3 = (int)((b & 0x00FF0000L) >> 16);
  b2 = (int)((b & 0x0000FF00L) >> 8);
  b1 = (int)b & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      if (fputc(b3, file) == b3)
        if (fputc(b4, file) == b4)
          return 0;

  return -1;
}

// Writes a 64-bit double-precision floating point number using
// little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputd(double l, FILE* file)
{
  const long long b = *(copy_reinterpret_cast<long long*>(&l));
  int b1, b2, b3, b4, b5, b6, b7, b8;

  // Little endian.
  b8 = (int)((b & 0xFF00000000000000L) >> 56);
  b7 = (int)((b & 0x00FF000000000000L) >> 48);
  b6 = (int)((b & 0x0000FF0000000000L) >> 40);
  b5 = (int)((b & 0x000000FF00000000L) >> 32);
  b4 = (int)((b & 0x00000000FF000000L) >> 24);
  b3 = (int)((b & 0x0000000000FF0000L) >> 16);
  b2 = (int)((b & 0x000000000000FF00L) >> 8);
  b1 = (int)b & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      if (fputc(b3, file) == b3)
        if (fputc(b4, file) == b4)
          if (fputc(b5, file) == b5)
            if (fputc(b6, file) == b6)
              if (fputc(b7, file) == b7)
                if (fputc(b8, file) == b8)
                  return 0;

  return -1;
}

} // namespace base
