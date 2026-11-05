# laf-ft

*laf-ft* is a FreeType and HarfBuzz wrapper for shaping Unicode text
correctly. This library might be replaced with an alternative way to
handle text on *laf-os* by integrating Skia, the native APIs to
paint text, and a way to create/use sprite sheet-based fonts.

## API Reference

* [ft::FaceFT](https://github.com/aseprite/laf/blob/main/ft/face.h): FT_Face wrapper
* [ft::ForEachGlyph](https://github.com/aseprite/laf/blob/main/ft/algorithm.h): Algorithm to iterate each glyph
* [ft::HBFace](https://github.com/aseprite/laf/blob/main/ft/hb_face.h): hb_font_t wrapper
* [ft::HBShaper](https://github.com/aseprite/laf/blob/main/ft/hb_shaper.h): hb_shape & hb_buffer wrappers
* [ft::Lib](https://github.com/aseprite/laf/blob/main/ft/lib.h): FT_Library wrapper
