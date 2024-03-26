// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/window.h"
#include "os/win/dnd.h"

#include <shlobj.h>

DWORD as_dropeffect(const os::DropOperation op)
{
  DWORD effect = DROPEFFECT_NONE;
  if (op & os::DropOperation::Copy)
    effect |= DROPEFFECT_COPY;

  if (op & os::DropOperation::Move)
    effect |= DROPEFFECT_MOVE;

  if (op & os::DropOperation::Link)
    effect |= DROPEFFECT_LINK;

  return effect;
}

os::DropOperation as_dropoperation(DWORD pdwEffect)
{
  int op = 0;
  if (pdwEffect & DROPEFFECT_COPY)
    op |= os::DropOperation::Copy;

  if (pdwEffect & DROPEFFECT_MOVE)
    op |= os::DropOperation::Move;

  if (pdwEffect & DROPEFFECT_LINK)
    op |= os::DropOperation::Link;

  return static_cast<os::DropOperation>(op);
}


gfx::Point drag_position(HWND hwnd, POINTL& pt)
{
  ScreenToClient(hwnd, (LPPOINT) &pt);
  return gfx::Point(pt.x, pt.y);
}

namespace os {

base::paths DragDataProviderWin::getPaths()
{
  base::paths files;
  FORMATETC fmt;
  ZeroMemory(&fmt, sizeof(fmt));
  fmt.cfFormat = CF_HDROP;
  fmt.tymed = TYMED::TYMED_HGLOBAL;
  STGMEDIUM medium;
  if (m_data->GetData(&fmt, &medium) == S_OK) {
    HDROP hdrop = (HDROP) GlobalLock(medium.hGlobal);
    if (hdrop) {
      int count = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0);
      for (int index = 0; index < count; ++index) {
        int length = DragQueryFile(hdrop, index, nullptr, 0);
        if (length > 0) {
          std::vector<TCHAR> str(length + 1);
          DragQueryFile(hdrop, index, &str[0], str.size());
          files.push_back(base::to_utf8(&str[0]));
        }
      }
      GlobalUnlock(medium.hGlobal);
    }
    ReleaseStgMedium(&medium);
  }
  return files;
}

SurfaceRef DragDataProviderWin::getImage()
{
  // The following code was adapted from the clip library:
  // https://github.com/aseprite/clip/blob/835cd0f7e7a964bb969482117856bc56a0ac12bf/clip_osx.mm#L29


  /*
  NSPasteboardType imageType = [m_pasteboard availableTypeFromArray:
                            [NSArray arrayWithObjects:
                              NSPasteboardTypePNG, NSPasteboardTypeTIFF, nil
                            ]];
  if (!imageType)
    return nullptr;

  NSData* data = [m_pasteboard dataForType:imageType];
  if (!data)
    return nullptr;

  NSBitmapImageRep* bitmap = [NSBitmapImageRep imageRepWithData:data];

  if ((bitmap.bitmapFormat & NSBitmapFormatFloatingPointSamples) ||
      (bitmap.planar))
    throw base::Exception("Image format is not supported");

  SurfaceFormatData sf;
  sf.bitsPerPixel = bitmap.bitsPerPixel;

  // We need three samples for Red/Green/Blue
  if (bitmap.samplesPerPixel >= 3) {
    // Here we are guessing the bits per sample (generally 8, not
    // sure how many bits per sample macOS uses for 16bpp
    // NSBitmapFormat or if this format is even used).
    int bits_per_sample = (bitmap.bitsPerPixel == 16 ? 5: 8);
    int bits_shift = 0;

    // With alpha
    if (bitmap.alpha) {
      if (bitmap.bitmapFormat & NSBitmapFormatAlphaFirst) {
        sf.alphaShift = 0;
        bits_shift += bits_per_sample;
      }
      else {
        sf.alphaShift = 3*bits_per_sample;
      }
    }

    uint32_t* masks = &sf.redMask;
    uint32_t* shifts = &sf.redShift;
    // Red/green/blue shifts
    for (uint32_t* shift=shifts; shift<shifts+3; ++shift) {
      *shift = bits_shift;
      bits_shift += bits_per_sample;
    }

    // With alpha
    if (bitmap.alpha) {
      if (bitmap.bitmapFormat & NSBitmapFormatSixteenBitBigEndian ||
          bitmap.bitmapFormat & NSBitmapFormatThirtyTwoBitBigEndian) {
        std::swap(sf.redShift, sf.alphaShift);
        std::swap(sf.greenShift, sf.blueShift);
      }
    }
    // Without alpha
    else {
      if (bitmap.bitmapFormat & NSBitmapFormatSixteenBitBigEndian ||
          bitmap.bitmapFormat & NSBitmapFormatThirtyTwoBitBigEndian) {
        std::swap(sf.redShift, sf.blueShift);
      }
    }

    // Calculate all masks
    for (uint32_t* shift=shifts, *mask=masks; shift<shifts+4; ++shift, ++mask)
      *mask = ((1ul<<bits_per_sample)-1ul) << (*shift);

    // Without alpha
    if (!bitmap.alpha)
      sf.alphaMask = 0;
  }

  if (bitmap.alpha) {
    sf.pixelAlpha = (bitmap.bitmapFormat & NSBitmapFormatAlphaNonpremultiplied
                    ? os::PixelAlpha::kStraight
                    : os::PixelAlpha::kPremultiplied);
  }
  else
    sf.pixelAlpha = os::PixelAlpha::kOpaque;

  SurfaceRef surface = os::instance()->makeSurface(
                          bitmap.pixelsWide,
                          bitmap.pixelsHigh,
                          sf,
                          bitmap.bitmapData
                        );
  return surface;
  */
  return nullptr;
}

bool DragDataProviderWin::contains(DragDataItemType type)
{
  IEnumFORMATETC* formats;
  if (m_data->EnumFormatEtc(DATADIR::DATADIR_GET, &formats) != S_OK)
    return false;

  TCHAR name[101];
  bool found = false;
  FORMATETC fmt;
  while (formats->Next(1, &fmt, nullptr) == S_OK) {
    if (type == DragDataItemType::Paths && fmt.cfFormat == CF_HDROP) {
      found = true;
      break;
    }

    if (type == DragDataItemType::Image) {
      if (fmt.cfFormat == CF_DIB || fmt.cfFormat == CF_DIBV5) {
        found = true;
        break;
      }
      else {
        int namelen = GetClipboardFormatName(fmt.cfFormat, name, 100);
        name[namelen] = '\0';
        if (lstrcmpi(name, L"PNG") == 0 || lstrcmpi(name, L"image/png") == 0) {
          found = true;
          break;
        }
      }

    }
  }

  formats->Release();
  return found;
}

STDMETHODIMP DragTargetAdapter::QueryInterface(REFIID riid, LPVOID* ppv)
{
  if (!ppv)
    return E_INVALIDARG;

  *ppv = nullptr;
  if (riid != IID_IDropTarget && riid != IID_IUnknown)
    return E_NOINTERFACE;

  *ppv = static_cast<IDropTarget*>(this);
  AddRef();
  return NOERROR;
}

ULONG DragTargetAdapter::AddRef()
{
  return InterlockedIncrement(&m_ref);
}

ULONG DragTargetAdapter::Release()
{
  // Decrement the object's internal counter.
  ULONG ref = InterlockedDecrement(&m_ref);
  if (0 == ref)
    delete this;

  return ref;
}

STDMETHODIMP DragTargetAdapter::DragEnter(IDataObject* pDataObj,
                                          DWORD grfKeyState,
                                          POINTL pt,
                                          DWORD* pdwEffect)
{
  if (!m_window->hasDragTarget())
    return E_NOTIMPL;

  m_data = base::ComPtr<IDataObject>(pDataObj);
  if (!m_data)
    return E_UNEXPECTED;

  m_position = drag_position((HWND) m_window->nativeHandle(), pt);
  auto ddProvider = std::make_unique<DragDataProviderWin>(m_data.get());
  DragEvent ev(m_window,
               as_dropoperation(*pdwEffect),
               m_position,
               ddProvider.get());

  m_window->notifyDragEnter(ev);

  *pdwEffect = as_dropeffect(ev.dropResult());

  return S_OK;
}

STDMETHODIMP DragTargetAdapter::DragOver(DWORD grfKeyState,
                                         POINTL pt,
                                         DWORD* pdwEffect)
{
  if (!m_window->hasDragTarget())
    return E_NOTIMPL;

  m_position = drag_position((HWND)m_window->nativeHandle(), pt);
  auto ddProvider = std::make_unique<DragDataProviderWin>(m_data.get());
  DragEvent ev(m_window,
               as_dropoperation(*pdwEffect),
               m_position,
               ddProvider.get());

  m_window->notifyDrag(ev);

  *pdwEffect = as_dropeffect(ev.dropResult());

  return S_OK;
}

STDMETHODIMP DragTargetAdapter::DragLeave(void)
{
  if (!m_window->hasDragTarget())
    return E_NOTIMPL;

  auto ddProvider = std::make_unique<DragDataProviderWin>(m_data.get());
  os::DragEvent ev(m_window,
                   DropOperation::None,
                   m_position,
                   ddProvider.get());
  m_window->notifyDragLeave(ev);

  m_data.reset();
  return S_OK;
}

STDMETHODIMP DragTargetAdapter::Drop(IDataObject* pDataObj,
                                     DWORD grfKeyState,
                                     POINTL pt,
                                     DWORD* pdwEffect)
{
  if (!m_window->hasDragTarget())
    return E_NOTIMPL;

  m_data = base::ComPtr<IDataObject>(pDataObj);
  if (!m_data)
    return E_UNEXPECTED;

  m_position = drag_position((HWND)m_window->nativeHandle(), pt);
  auto ddProvider = std::make_unique<DragDataProviderWin>(m_data.get());
  DragEvent ev(m_window,
               as_dropoperation(*pdwEffect),
               m_position,
               ddProvider.get());

  m_window->notifyDrop(ev);

  m_data = nullptr;
  *pdwEffect = as_dropeffect(ev.dropResult());
  return S_OK;
}


} // namespase os
