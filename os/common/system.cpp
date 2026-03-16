// LAF OS Library
// Copyright (C) 2019-2025  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/common/system.h"

#if CLIP_ENABLE_IMAGE
  #include "clip/clip.h"
#endif

#include "base/debug.h"

namespace os {

// Weak reference to the unique system instance. This is destroyed by
// the user (with the main SystemRef to the system).
static System* g_instance = nullptr;

// Flag to know if the intance is already being destroyed, so we
// cannot add a ref to it, i.e. calling System::instance() is illegal
// if this flag is true.
static bool g_is_being_destroyed = false;

System* System::rawInstance()
{
  return g_instance;
}

SystemRef System::instance()
{
  ASSERT(!g_is_being_destroyed);
  if (g_instance)
    return AddRef(g_instance);
  return nullptr;
}

SystemRef System::make()
{
  ASSERT(!g_instance);

  SystemRef ref;
#if LAF_SKIA
  ref = System::makeSkia();
#endif
#if LAF_WINDOWS
  if (!ref)
    ref = System::makeWin();
#elif LAF_IOS
  if (!ref)
    ref = System::makeIOS();
#elif LAF_MACOS
  if (!ref)
    ref = System::makeOSX();
#elif LAF_LINUX
  if (!ref)
    ref = System::makeX11();
#endif
  if (!ref)
    ref = System::makeNone();

  if (!g_instance)
    g_instance = ref.get();

  return ref;
}

CommonSystem::CommonSystem()
{
  g_is_being_destroyed = false;
}

CommonSystem::~CommonSystem()
{
  // destroyInstance() can be called multiple times by derived
  // classes.
  if (g_instance == this)
    destroyInstance();
}

KeyModifiers CommonSystem::keyModifiers()
{
  return (
    KeyModifiers)((isKeyPressed(kKeyLShift) || isKeyPressed(kKeyRShift) ? kKeyShiftModifier : 0) |
                  (isKeyPressed(kKeyLControl) || isKeyPressed(kKeyRControl) ? kKeyCtrlModifier :
                                                                              0) |
                  (isKeyPressed(kKeyAlt) ? kKeyAltModifier : 0) |
                  (isKeyPressed(kKeyAltGr) ? (kKeyCtrlModifier | kKeyAltModifier) : 0) |
                  (isKeyPressed(kKeyCommand) ? kKeyCmdModifier : 0) |
                  (isKeyPressed(kKeySpace) ? kKeySpaceModifier : 0) |
                  (isKeyPressed(kKeyLWin) || isKeyPressed(kKeyRWin) ? kKeyWinModifier : 0));
}

#if CLIP_ENABLE_IMAGE

void get_rgba32(const clip::image_spec& spec,
                const uint8_t* scanlineAddr,
                int* r,
                int* g,
                int* b,
                int* a)
{
  uint32_t c = *((uint32_t*)scanlineAddr);
  if (spec.alpha_mask)
    *a = ((c & spec.alpha_mask) >> spec.alpha_shift);
  else
    *a = 255;
  // The source image is in straight-alpha and makeRgbaSurface returns a
  // surface using premultiplied-alpha so we have to premultiply the
  // source values.
  *r = ((c & spec.red_mask) >> spec.red_shift) * (*a) / 255;
  *g = ((c & spec.green_mask) >> spec.green_shift) * (*a) / 255;
  *b = ((c & spec.blue_mask) >> spec.blue_shift) * (*a) / 255;
}

void get_rgba24(const clip::image_spec& spec,
                const uint8_t* scanlineAddr,
                int* r,
                int* g,
                int* b,
                int* a)
{
  uint32_t c = *((uint32_t*)scanlineAddr);
  *r = ((c & spec.red_mask) >> spec.red_shift);
  *g = ((c & spec.green_mask) >> spec.green_shift);
  *b = ((c & spec.blue_mask) >> spec.blue_shift);
  *a = 255;
}

void get_rgba16(const clip::image_spec& spec,
                const uint8_t* scanlineAddr,
                int* r,
                int* g,
                int* b,
                int* a)
{
  uint16_t c = *((uint16_t*)scanlineAddr);
  *r = (((c & spec.red_mask) >> spec.red_shift) * 255) / (spec.red_mask >> spec.red_shift);
  *g = (((c & spec.green_mask) >> spec.green_shift) * 255) / (spec.green_mask >> spec.green_shift);
  *b = (((c & spec.blue_mask) >> spec.blue_shift) * 255) / (spec.blue_mask >> spec.blue_shift);
  *a = 255;
}

SurfaceRef CommonSystem::makeSurface(const clip::image& image)
{
  const clip::image_spec spec = image.spec();

  if (spec.bits_per_pixel != 32 && spec.bits_per_pixel != 24 && spec.bits_per_pixel != 16)
    return nullptr;

  SurfaceRef surface = ((System*)this)->makeRgbaSurface(spec.width, spec.height);
  SurfaceFormatData sfd;
  surface->getFormat(&sfd);

  // Select color components retrieval function.
  void (*get_rgba)(const clip::image_spec&, const uint8_t*, int*, int*, int*, int*);
  switch (spec.bits_per_pixel) {
    case 32: get_rgba = get_rgba32; break;
    case 24: get_rgba = get_rgba24; break;
    case 16: get_rgba = get_rgba16; break;
  }

  for (int v = 0; v < spec.height; ++v) {
    uint32_t* dst = (uint32_t*)surface->getData(0, v);
    const uint8_t* src = ((uint8_t*)image.data()) + v * spec.bytes_per_row;
    for (int u = 0; u < spec.width; ++u, ++dst) {
      int r, g, b, a;
      get_rgba(spec, src, &r, &g, &b, &a);
      *dst = (r << sfd.redShift) | (g << sfd.greenShift) | (b << sfd.blueShift) |
             (a << sfd.alphaShift);

      src += (spec.bits_per_pixel / 8);
    }
  }

  return surface;
}

#endif

// This must be called in the final class that derived from
// CommonSystem, because clearing the list of events can generate
// events on windows that will depend on the platform-specific
// System.
//
// E.g. We've crash reports because WindowWin is receiving
// WM_ACTIVATE messages when we destroy the events queue, and the
// handler of that message is expecting the SystemWin instance (not
// a CommonSystem instance). That's why we cannot call this from
// ~CommonSystem() destructor and we have to call this from
// ~SystemWin (or other platform-specific System implementations).
void CommonSystem::destroyInstance()
{
  // destroyInstance() can be called multiple times by derived
  // classes.
  if (g_instance != this) {
    ASSERT(g_is_being_destroyed);
    return;
  }

  g_is_being_destroyed = true;

  // We have to reset the list of all events to clear all possible
  // living WindowRef (so all window destructors are called at this
  // point, when the os::System instance is still alive).
  //
  // TODO Maybe the event queue should be inside the System instance
  //      (so when the system is deleted, the queue is
  //      deleted). Anyway we should still clear all the events
  //      before g_instance=nullptr, and we're not sure if this
  //      is possible on macOS, as some events are queued before the
  //      System instance is even created (see
  //      EventQueue::instance() comment on laf/os/event_queue.h).
  eventQueue()->clearEvents();

  g_instance = nullptr;
}

} // namespace os
