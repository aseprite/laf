// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_DND_H_INCLUDED
#define OS_DND_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "base/debug.h"
#include "gfx/point.h"
#include "os/surface.h"

#include <memory>

#pragma push_macro("None")
#undef None // Undefine the X11 None macro

namespace os {

  class Window;

  // Operations that can be supported by source and target windows in a drag
  // and drop operation.
  enum DropOperation {
    None = 0,
    Copy = 1,
    Move = 2,
    Link = 4,
    Any = Copy | Move | Link,
  };

  // Types of representations supported for each DragDataItem.
  enum class DragDataItemType {
    Paths,
    Image,
  };

  // Interface to get dragged data from the platform's implementation.
  class DragDataProvider {
  public:
    virtual ~DragDataProvider() {}
    virtual base::paths getPaths() = 0;
    virtual SurfaceRef getImage() = 0;
    virtual bool contains(DragDataItemType type) { return false; }
  };

  class DragEvent {
  public:
    DragEvent(os::Window* target,
              DropOperation supportedOperations,
              const gfx::Point& dragPosition,
              DragDataProvider* dataProvider)
      : m_target(target)
      , m_supportedOperations(supportedOperations)
      , m_position(dragPosition)
      , m_dataProvider(dataProvider) {}

    // Destination window of the DragEvent.
    os::Window* target() const { return m_target; }
    DropOperation dropResult() const { return m_dropResult; }
    DropOperation supportedOperations() const { return m_supportedOperations; }
    bool acceptDrop() const { return m_acceptDrop; }
    const gfx::Point& position() { return m_position; }
    DragDataProvider* dataProvider() { return m_dataProvider; }

    // Sets what will be the outcome of dropping the dragged data when it is
    // accepted by the target window. Only one of the enum values should be passed,
    // do not combine values using bitwise operators.
    void dropResult(DropOperation operation) { m_dropResult = operation; }
    // Set this to true when the dropped data was accepted/processed by the
    // target window, or set to false otherwise.
    void acceptDrop(bool value) { m_acceptDrop = value; }

    bool sourceSupports(DropOperation op) { return (m_supportedOperations & op) == op; }

  private:
    os::Window* m_target;
    DropOperation m_dropResult = DropOperation::Copy;
    // Bitwise OR of the operations supported by the drag and drop source.
    DropOperation m_supportedOperations;
    bool m_acceptDrop = false;
    gfx::Point m_position;
    DragDataProvider* m_dataProvider;
  };

  class DragTarget {
  public:
    virtual ~DragTarget() {};

    // Called when a drag action enters a window that supports DnD. The
    // DragEvent::dropResult must be set to the operation that is expected
    // to occur by the target window once the drop is accepted.
    virtual void dragEnter(os::DragEvent& ev) {}
    // Called when the dragged data exits the window that supports DnD.
    virtual void dragLeave(os::DragEvent& ev) {}
    virtual void drag(os::DragEvent& ev) {}
    virtual void drop(os::DragEvent& ev) {}
  };


} // namespace os

#pragma pop_macro("None")

#endif