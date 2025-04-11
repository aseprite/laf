// LAF Text Library
// Copyright (c) 2019-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "text/skia_font_mgr.h"

#include "text/skia_font.h"

#include "include/core/SkFont.h"
#include "include/core/SkString.h"

#if LAF_WINDOWS
  #include "include/ports/SkTypeface_win.h"
#elif LAF_MACOS
  #include "include/ports/SkFontMgr_mac_ct.h"
#elif LAF_LINUX
  #include "include/ports/SkFontMgr_fontconfig.h"
  #include "include/ports/SkFontScanner_FreeType.h"
#endif

namespace text {

//////////////////////////////////////////////////////////////////////
// SkiaTypeface

struct SkiaFontStyleSet::LockSet {
  LockSet(SkiaFontStyleSet* set) : m_set(set)
  {
    if (m_set)
      m_set->m_mutex.lock();
  }
  ~LockSet()
  {
    if (m_set)
      m_set->m_mutex.unlock();
  }
  SkiaFontStyleSet* m_set;
};

SkiaTypeface::SkiaTypeface(sk_sp<SkTypeface> skTypeface, SkiaFontStyleSet* owner)
  : m_skTypeface(skTypeface)
  , m_owner(owner)
{
}

std::string SkiaTypeface::familyName() const
{
  SkiaFontStyleSet::LockSet lock(m_owner);
  SkString name;
  m_skTypeface->getFamilyName(&name);
  return std::string(name.c_str());
}

FontStyle SkiaTypeface::fontStyle() const
{
  SkiaFontStyleSet::LockSet lock(m_owner);
  SkFontStyle skStyle = m_skTypeface->fontStyle();
  return FontStyle((FontStyle::Weight)skStyle.weight(),
                   (FontStyle::Width)skStyle.width(),
                   (FontStyle::Slant)skStyle.slant());
}

//////////////////////////////////////////////////////////////////////
// SkiaFontStyleSet

SkiaFontStyleSet::SkiaFontStyleSet(sk_sp<SkFontStyleSet> set) : m_skSet(set)
{
}

int SkiaFontStyleSet::count()
{
  LockSet lock(this);
  return m_skSet->count();
}

void SkiaFontStyleSet::getStyle(int index, FontStyle& style, std::string& name)
{
  LockSet lock(this);
  SkFontStyle skStyle;
  SkString skName;
  m_skSet->getStyle(index, &skStyle, &skName);
  style = FontStyle((FontStyle::Weight)skStyle.weight(),
                    (FontStyle::Width)skStyle.width(),
                    (FontStyle::Slant)skStyle.slant());
  name = skName.c_str();
}

TypefaceRef SkiaFontStyleSet::typeface(int index)
{
  LockSet lock(this);
  return base::make_ref<SkiaTypeface>(m_skSet->createTypeface(index), this);
}

TypefaceRef SkiaFontStyleSet::matchStyle(const FontStyle& style)
{
  LockSet lock(this);
  SkFontStyle skStyle((SkFontStyle::Weight)style.weight(),
                      (SkFontStyle::Width)style.width(),
                      (SkFontStyle::Slant)style.slant());
  return base::make_ref<SkiaTypeface>(m_skSet->matchStyle(skStyle), this);
}

//////////////////////////////////////////////////////////////////////
// SkiaFontMgr

// static
FontMgrRef FontMgr::Make()
{
  return base::make_ref<SkiaFontMgr>();
}

SkiaFontMgr::SkiaFontMgr()
{
#if LAF_WINDOWS
  m_skFontMgr = SkFontMgr_New_DirectWrite();
#elif LAF_MACOS
  m_skFontMgr = SkFontMgr_New_CoreText(nullptr);
#elif LAF_LINUX
  m_skFontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#endif
  if (!m_skFontMgr)
    m_skFontMgr = SkFontMgr::RefEmpty();
}

SkiaFontMgr::~SkiaFontMgr()
{
}

FontRef SkiaFontMgr::loadTrueTypeFont(const char* filename, float size)
{
  // Use the native impl from Skia to load the font file
  sk_sp<SkTypeface> face = m_skFontMgr->makeFromFile(filename);
  if (!face) {
    // In other case try the FreeType impl
    return FontMgr::loadTrueTypeFont(filename, size);
  }

  SkFont skFont(face, size);
  return base::make_ref<SkiaFont>(skFont);
}

FontRef SkiaFontMgr::defaultFont(float size) const
{
  sk_sp<SkTypeface> face = m_skFontMgr->legacyMakeTypeface(nullptr, SkFontStyle());
  ASSERT(face);
  SkFont skFont(face, size);
  return base::make_ref<SkiaFont>(skFont);
}

FontRef SkiaFontMgr::makeFont(const TypefaceRef& typeface)
{
  ASSERT(typeface.get());
  return base::make_ref<SkiaFont>(SkFont(static_cast<SkiaTypeface*>(typeface.get())->skTypeface()));
}

FontRef SkiaFontMgr::makeFont(const TypefaceRef& typeface, float size)
{
  ASSERT(typeface.get());
  return base::make_ref<SkiaFont>(
    SkFont(static_cast<SkiaTypeface*>(typeface.get())->skTypeface(), size));
}

int SkiaFontMgr::countFamilies() const
{
  return m_skFontMgr->countFamilies();
}

std::string SkiaFontMgr::familyName(int i) const
{
  SkString name;
  m_skFontMgr->getFamilyName(i, &name);
  return std::string(name.c_str());
}

FontStyleSetRef SkiaFontMgr::familyStyleSet(int i) const
{
  return base::make_ref<SkiaFontStyleSet>(m_skFontMgr->createStyleSet(i));
}

FontStyleSetRef SkiaFontMgr::matchFamily(const std::string& familyName) const
{
  auto set = m_skFontMgr->matchFamily(familyName.c_str());

  // Not sure why Skia (at least the SkFontMgr_DirectWrite impl, the
  // IDWriteFontCollection::FindFamilyName method) returns a valid
  // pointer even when the font family doesn't exist.
  if (!set || set->count() == 0)
    return nullptr;

  return base::make_ref<SkiaFontStyleSet>(set);
}

} // namespace text
