# laf-base

Cross-platform core functionality to do basic tasks. Ideally this
functionality would be included in the C++ standard library, so some
functions may be replaced with a `std::` equivalent in the future.

* Data utilities ([encode/decode_base64](https://github.com/aseprite/laf/blob/main/base/base64.h))
* File system & filename/path utilities ([fs.h](https://github.com/aseprite/laf/blob/main/base/fs.h))
* File utilities
  ([serialization](https://github.com/aseprite/laf/blob/main/base/serialization.h),
  [sha1](https://github.com/aseprite/laf/blob/main/base/sha1.h),
  [launcher](https://github.com/aseprite/laf/blob/main/base/launcher.h))
* Logging functions ([LOG()](https://github.com/aseprite/laf/blob/main/base/log.h))
* Manage DLLs ([load/unload_dll()](https://github.com/aseprite/laf/blob/main/base/dll.h))
* Multi-threading utilities ([thread](https://github.com/aseprite/laf/blob/main/base/thread.h),
  [thread_pool](https://github.com/aseprite/laf/blob/main/base/thread_pool.h))
* Smart pointers ([RefCount/Ref](https://github.com/aseprite/laf/blob/main/base/ref.h))
* String/UTF8 utilities
  ([string](https://github.com/aseprite/laf/blob/main/base/string.h),
  [split_string](https://github.com/aseprite/laf/blob/main/base/split_string.h),
  [trim_string](https://github.com/aseprite/laf/blob/main/base/trim_string.h))
* Timing ([tick_t/current_tick()](https://github.com/aseprite/laf/blob/main/base/time.h),
  [Chrono](https://github.com/aseprite/laf/blob/main/base/chrono.h))
* Type conversion ([convert_to](https://github.com/aseprite/laf/blob/main/base/convert_to.h))
* Unicode filenames
  ([open_file_raw()](https://github.com/aseprite/laf/blob/main/base/file_handle.h),
  [FSTREAM_PATH()](https://github.com/aseprite/laf/blob/main/base/fstream_path.h))
* Version comparison
  ([Version](https://github.com/aseprite/laf/blob/main/base/version.h))
