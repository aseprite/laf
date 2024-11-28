// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_DND_H_INCLUDED
#define OS_WIN_DND_H_INCLUDED
#pragma once

#include "base/win/comptr.h"
#include "os/dnd.h"

#include <ole2.h>

namespace os {

  class DragDataProviderWin : public DragDataProvider {
  public:
    DragDataProviderWin(IDataObject* pDataObj) : m_data(pDataObj) { }

  private:
    IDataObject* m_data;

    base::paths getPaths() override;
#if CLIP_ENABLE_IMAGE
    SurfaceRef getImage() override;
#endif
    std::string getUrl() override;
    bool contains(DragDataItemType type) override;
  };

  // IDropTarget implementation used to adapt the OLE's Drag & Drop interface to
  // Laf's DragTarget interface.
  class DragTargetAdapter : public IDropTarget {
  public:
    DragTargetAdapter(Window* window) : m_window(window) { }

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppv) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IDropTarget methods
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj,
                                        DWORD grfKeyState,
                                        POINTL pt,
                                        DWORD* pdwEffect) override;
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState,
                                       POINTL pt,
                                       DWORD* pdwEffect) override;
    HRESULT STDMETHODCALLTYPE DragLeave(void) override;
    HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj,
                                   DWORD grfKeyState,
                                   POINTL pt,
                                   DWORD* pdwEffect) override;

  private:
    DragEvent newDragEvent(POINTL* pt, DWORD* pdwEffect);

    ULONG m_ref = 0;
    Window* m_window = nullptr;
    // Pointer to data being dragged
    base::ComPtr<IDataObject> m_data;
    // Last mouse position when dragging data over the target.
    gfx::Point m_position = gfx::Point(0, 0);
  };
} // namespace os

#endif
