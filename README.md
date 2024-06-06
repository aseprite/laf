# LAF: The Lost Application Framework

Testing

[![build](https://github.com/aseprite/laf/workflows/build/badge.svg)](https://github.com/aseprite/laf/actions?query=workflow%3Abuild)
[![MIT Licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.txt)

A library to create Windows, macOS, and Linux applications.

This library is under active development so we don't provide API or
ABI compatibility at this moment.

* [Documentation](https://aseprite.github.io/laf/)

## Dependencies

*laf* can be compiled with two back-ends (`LAF_BACKEND`): `skia` or `none`.

When `LAF_BACKEND=skia`, *laf* requires a compiled version of the [Skia library](https://skia.org/)
from branch `aseprite-m102`. You can check the aseprite/skia fork
which includes a [release with pre-built versions](https://github.com/aseprite/skia/releases), or
the check the [instructions to compile skia](https://github.com/aseprite/skia#readme) from scratch.

When `LAF_BACKEND=none`, the [Pixman library](http://www.pixman.org/)
can be used as an alternative implementation of the `gfx::Region` class (generally if
you're using `laf-os` you will link it with Skia, so there is no
need for Pixman at all).

## Compile

To compile *laf* with Skia as backend you have to specify some
variables pointing to a valid compiled version of Skia in your
disk. In the following example `/skiadir` is the absolute path to a
directory where the Skia source code + compiled version is, or just
where you've uncompressed a pre-built package of Skia (note that in
this case `/skiadir/out/Release-x64` should contain the Skia library
files, i.e. `skia.lib` on Windows or `libskia.a` on other platforms):

```
git clone https://github.com/aseprite/laf.git
cd laf
mkdir build
cd build
cmake -G Ninja \
  -DLAF_BACKEND=skia \
  -DSKIA_DIR=/skiadir \
  -DSKIA_LIBRARY_DIR=/skiadir/out/Release-x64 \
  ..
ninja
./examples/helloworld
```

To compile only the library (without examples and tests) you can
disable the `LAF_WITH_EXAMPLES`/`LAF_WITH_TESTS` options:

```
cmake -DLAF_WITH_EXAMPLES=OFF -DLAF_WITH_TESTS=OFF ...
```

## Running Tests

You can use `ctest` to run all tests:

```
cd build
ninja
ctest
```

## License

*laf* is distributed under the terms of [the MIT license](LICENSE.txt).

Some functions in *laf* depends on third party libraries (you should
include these license notices when you distribute your software):

* Tests use the [Google Test](https://github.com/aseprite/googletest/tree/master/googletest)
  framework by Google Inc. licensed under
  [a BSD-like license](https://github.com/aseprite/googletest/blob/master/googletest/LICENSE).
* Color spaces, `gfx::Region`, and the `laf::os` library use code from
  the [Skia library](https://skia.org) by Google Inc. licensed under
  [a BSD-like license](https://github.com/aseprite/skia/blob/master/LICENSE)
  and several other [third-party libraries/licenses](https://github.com/aseprite/skia/tree/master/third_party).
* `gfx::Region` uses the pixman library if you are not compiling with
  the Skia backend (e.g. a if you want to create only Command Line
  utilities that uses the `gfx::Region` class) on Linux or macOS. On
  Windows we use an alternative implementation with [HRGN](https://learn.microsoft.com/en-us/windows/win32/gdi/regions).
  Pixman is distributed under the [MIT License](https://cgit.freedesktop.org/pixman/tree/COPYING).
