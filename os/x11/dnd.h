// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_DND_INCLUDED
#define OS_X11_DND_INCLUDED
#pragma once

#include "os/dnd.h"
#include "os/x11/x11.h"

#include <string>
#include <vector>

namespace os {

struct DndDataX11 {
  ::Window sourceWindow = 0;
  gfx::Point position;
  DropOperation supportedOperations = DropOperation::None;
  DropOperation selectedOperation = DropOperation::None;
  std::vector<Atom> types;
  std::vector<Atom> targetFallbacks;
  Time dropTime = CurrentTime;
  int targetFallbackIndex = 0;
  bool dropInProgress = false;

  bool containsType(Atom dataType)
  {
    return (std::find(types.begin(), types.end(), dataType) != types.end());
  }
};

class DragDataProviderX11 : public DragDataProvider {
public:
  DragDataProviderX11(::Display* display, ::Window target, const base::paths& urls);

  base::paths getPaths() override;
#if CLIP_ENABLE_IMAGE
  SurfaceRef getImage() override;
#endif
  std::string getUrl() override;
  bool contains(DragDataItemType type) override;

private:
  ::Display* m_display;
  ::Window m_target;
  ::Window m_source;
  base::paths m_urls;
};

} // namespace os

#endif
