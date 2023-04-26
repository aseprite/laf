// LAF OS Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#define KEY_TRACE(...)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/osx/view.h"

#include "base/debug.h"
#include "gfx/point.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/osx/generate_drop_files.h"
#include "os/osx/keys.h"
#include "os/osx/window.h"
#include "os/system.h"

namespace os {

// Global variable used between View and NSMenuOSX to check if the
// keyDown: event was used by a key equivalent in the menu.
//
// TODO I'm not proud of this, but it does the job
bool g_keyEquivalentUsed = false;

bool g_async_view = true;

bool osx_is_key_pressed(KeyScancode scancode);

namespace {

// Internal array of pressed keys used in isKeyPressed()
int g_pressedKeys[kKeyScancodes];
bool g_translateDeadKeys = false;
UInt32 g_lastDeadKeyState = 0;
NSCursor* g_emptyNsCursor = nil;
Event::MouseButton g_lastMouseButton = Event::NoneButton;

gfx::Point get_local_mouse_pos(NSView* view, NSEvent* event)
{
  NSPoint point = [view convertPoint:[event locationInWindow]
                            fromView:nil];
  int scale = 1;
  if ([view window])
    scale = [(WindowOSXObjc*)[view window] scale];

  // "os" layer coordinates expect (X,Y) origin at the top-left corner.
  return gfx::Point(point.x / scale,
                    (view.bounds.size.height - point.y) / scale);
}

Event::MouseButton get_mouse_buttons(NSEvent* event)
{
  // Some Wacom drivers on OS X report right-clicks with
  // buttonNumber=0, so we've to check the type event anyway.
  switch (event.type) {
    case NSEventTypeLeftMouseDown:
    case NSEventTypeLeftMouseUp:
    case NSEventTypeLeftMouseDragged:
      return Event::LeftButton;
    case NSEventTypeRightMouseDown:
    case NSEventTypeRightMouseUp:
    case NSEventTypeRightMouseDragged:
      return Event::RightButton;
  }

  switch (event.buttonNumber) {
    case 0: return Event::LeftButton; break;
    case 1: return Event::RightButton; break;
    case 2: return Event::MiddleButton; break;
    // NSOtherMouseDown/Up/Dragged
    case 3: return Event::X1Button; break;
    case 4: return Event::X2Button; break;
  }

  return Event::MouseButton::NoneButton;
}

KeyModifiers get_modifiers_from_nsevent(NSEvent* event)
{
  int modifiers = kKeyNoneModifier;
  NSEventModifierFlags nsFlags = event.modifierFlags;
  if (nsFlags & NSEventModifierFlagShift) modifiers |= kKeyShiftModifier;
  if (nsFlags & NSEventModifierFlagControl) modifiers |= kKeyCtrlModifier;
  if (nsFlags & NSEventModifierFlagOption) modifiers |= kKeyAltModifier;
  if (nsFlags & NSEventModifierFlagCommand) modifiers |= kKeyCmdModifier;
  if (osx_is_key_pressed(kKeySpace)) modifiers |= kKeySpaceModifier;
  return (KeyModifiers)modifiers;
}

} // anonymous namespace

bool osx_is_key_pressed(KeyScancode scancode)
{
  if (scancode >= 0 && scancode < kKeyScancodes)
    return (g_pressedKeys[scancode] != 0);
  else
    return false;
}

int osx_get_unicode_from_scancode(KeyScancode scancode)
{
  if (scancode >= 0 && scancode < kKeyScancodes)
    return g_pressedKeys[scancode];
  else
    return 0;
}

void osx_set_async_view(bool state)
{
  g_async_view = state;
}

} // namespace os

using namespace os;

@implementation ViewOSX

- (id)initWithFrame:(NSRect)frameRect
{
  // This autoreleasepool with the removeImpl code is needed to
  // release CALayers from memory. Without this we will keep growing
  // the memory with CALayers.
  @autoreleasepool {
    // We start without the system mouse cursor
    m_nsCursor = nil;
    m_visibleMouse = true;
    m_pointerType = os::PointerType::Unknown;
    m_impl = nullptr;

    self = [super initWithFrame:frameRect];
    if (self != nil) {
      [self createMouseTrackingArea];
      [self registerForDraggedTypes:
              [NSArray arrayWithObjects:
                         NSFilenamesPboardType,
                       nil]];

      // Create a CALayer for backing content with async drawing. This
      // fixes performance issues on Retina displays with wide color
      // spaces (like Display P3).
      if (os::g_async_view) {
        self.wantsLayer = true;
        self.layer.drawsAsynchronously = true;
      }
    }
  }
  return self;
}

- (void)dealloc
{
  [self destroyMouseTrackingArea];
}

- (void)removeImpl
{
  @autoreleasepool {
    // Reconfigure the view to release the CALayer object. This along
    // with the autoreleasepool in initWithFrame: are needed.
    self.layer.drawsAsynchronously = false;
    self.wantsLayer = false;

    m_impl = nullptr;
  }
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
  [super acceptsFirstMouse:event];
  return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
  return YES;
}

- (void)viewDidChangeBackingProperties
{
  [super viewDidChangeBackingProperties];
  if (m_impl)
    m_impl->onChangeBackingProperties();
}

- (void)viewDidHide
{
  [super viewDidHide];
  [self destroyMouseTrackingArea];
}

- (void)viewDidUnhide
{
  [super viewDidUnhide];
  [self createMouseTrackingArea];
}

- (void)viewDidMoveToWindow
{
  [super viewDidMoveToWindow];

  if ([self window]) {
    m_impl = [((WindowOSXObjc*)[self window]) impl];
    if (m_impl)
      m_impl->onWindowChanged();
  }
  else
    m_impl = nullptr;
}

- (void)drawRect:(NSRect)dirtyRect
{
  [super drawRect:dirtyRect];
  if (m_impl)
    m_impl->onDrawRect(gfx::Rect(dirtyRect.origin.x,
                                 dirtyRect.origin.y,
                                 dirtyRect.size.width,
                                 dirtyRect.size.height));
}

- (void)keyDown:(NSEvent*)event
{
  g_keyEquivalentUsed = false;
  [super keyDown:event];

  // If a key equivalent used the keyDown event, we don't generate
  // this os::KeyDown event.
  if (g_keyEquivalentUsed)
    return;

  KeyScancode scancode = scancode_from_nsevent(event);
  Event ev;
  ev.setType(Event::KeyDown);
  ev.setScancode(scancode);
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setRepeat(event.ARepeat ? 1: 0);
  ev.setUnicodeChar(0);

  bool sendMsg = true;

  CFStringRef strRef = get_unicode_from_key_code(event.keyCode,
                                                 event.modifierFlags);
  if (strRef) {
    int length = CFStringGetLength(strRef);
    if (length == 1)
      ev.setUnicodeChar(CFStringGetCharacterAtIndex(strRef, 0));
    CFRelease(strRef);
  }

  if (scancode >= 0 && scancode < kKeyScancodes)
    g_pressedKeys[scancode] = (ev.unicodeChar() ? ev.unicodeChar(): 1);

  if (g_translateDeadKeys) {
    strRef = get_unicode_from_key_code(event.keyCode,
                                       event.modifierFlags,
                                       &g_lastDeadKeyState);
    if (strRef) {
      int length = CFStringGetLength(strRef);
      if (length > 0) {
        sendMsg = false;
        for (int i=0; i<length; ++i) {
          ev.setUnicodeChar(CFStringGetCharacterAtIndex(strRef, i));
          [self queueEvent:ev];
        }
        g_lastDeadKeyState = 0;
      }
      else {
        ev.setDeadKey(true);
      }
      CFRelease(strRef);
    }
  }

  KEY_TRACE("View keyDown: unicode=%d (%c) scancode=%d modifiers=%d\n",
            ev.unicodeChar(), ev.unicodeChar(),
            ev.scancode(), ev.modifiers());

  if (sendMsg)
    [self queueEvent:ev];
}

- (void)keyUp:(NSEvent*)event
{
  [super keyUp:event];

  KeyScancode scancode = scancode_from_nsevent(event);
  if (scancode >= 0 && scancode < kKeyScancodes)
    g_pressedKeys[scancode] = 0;

  Event ev;
  ev.setType(Event::KeyUp);
  ev.setScancode(scancode);
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setRepeat(event.ARepeat ? 1: 0);
  ev.setUnicodeChar(0);

  [self queueEvent:ev];
}

- (void)flagsChanged:(NSEvent*)event
{
  [super flagsChanged:event];
  [ViewOSX updateKeyFlags:event];
}

+ (void)updateKeyFlags:(NSEvent*)event
{
  static int lastFlags = 0;
  static int flags[] = {
    NSEventModifierFlagShift,
    NSEventModifierFlagControl,
    NSEventModifierFlagOption,
    NSEventModifierFlagCommand
  };
  static KeyScancode scancodes[] = {
    kKeyLShift,
    kKeyLControl,
    kKeyAlt,
    kKeyCommand
  };

  KeyModifiers modifiers = get_modifiers_from_nsevent(event);
  int newFlags = event.modifierFlags;

  for (int i=0; i<sizeof(flags)/sizeof(flags[0]); ++i) {
    if ((lastFlags & flags[i]) != (newFlags & flags[i])) {
      Event ev;
      ev.setType(
        ((newFlags & flags[i]) != 0 ? Event::KeyDown:
                                      Event::KeyUp));

      g_pressedKeys[scancodes[i]] = ((newFlags & flags[i]) != 0);

      ev.setScancode(scancodes[i]);
      ev.setModifiers(modifiers);
      ev.setRepeat(0);
      // TODO send one message to each display? use [... queueEvent:ev] in some way
      os::queue_event(ev);
    }
  }

  lastFlags = newFlags;
}

- (void)mouseEntered:(NSEvent*)event
{
  [self updateCurrentCursor];

  Event ev;
  ev.setType(Event::MouseEnter);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  [self queueEvent:ev];
}

- (void)mouseMoved:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setPressure(event.pressure);

  if (m_pointerType != os::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  [self queueEvent:ev];
}

- (void)mouseExited:(NSEvent*)event
{
  // Restore arrow cursor
  if (!m_visibleMouse)
    m_visibleMouse = true;
  [[NSCursor arrowCursor] set];

  Event ev;
  ev.setType(Event::MouseLeave);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  [self queueEvent:ev];
}

- (void)mouseDown:(NSEvent*)event
{
  [self handleMouseDown:event];
}

- (void)mouseUp:(NSEvent*)event
{
  [self handleMouseUp:event];
}

- (void)mouseDragged:(NSEvent*)event
{
  [self handleMouseDragged:event];
}

- (void)rightMouseDown:(NSEvent*)event
{
  [self handleMouseDown:event];
}

- (void)rightMouseUp:(NSEvent*)event
{
  [self handleMouseUp:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
  [self handleMouseDragged:event];
}

- (void)otherMouseDown:(NSEvent*)event
{
  [self handleMouseDown:event];
}

- (void)otherMouseUp:(NSEvent*)event
{
  [self handleMouseUp:event];
}

- (void)otherMouseDragged:(NSEvent*)event
{
  [self handleMouseDragged:event];
}

- (void)handleMouseDown:(NSEvent*)event
{
  Event::MouseButton button = get_mouse_buttons(event);
  Event ev;

  if (event.clickCount == 2 &&
      button == g_lastMouseButton) {
    ev.setType(Event::MouseDoubleClick);
  }
  else {
    ev.setType(Event::MouseDown);
    g_lastMouseButton = button;
  }

  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(button);
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setPressure(event.pressure);

  if (m_pointerType != os::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  [self queueEvent:ev];
}

- (void)handleMouseUp:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseUp);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setPressure(event.pressure);

  if (m_pointerType != os::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  [self queueEvent:ev];
}

- (void)handleMouseDragged:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseMove);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setPressure(event.pressure);

  if (m_pointerType != os::PointerType::Unknown)
    ev.setPointerType(m_pointerType);

  [self queueEvent:ev];
}

- (void)setFrameSize:(NSSize)newSize
{
  [super setFrameSize:newSize];

  // Re-create the mouse tracking area
  [self destroyMouseTrackingArea];
  [self createMouseTrackingArea];

  // Call WindowOSX::onResize handler
  if (m_impl) {
    m_impl->onResize(gfx::Size(newSize.width,
                               newSize.height));
  }
}

- (void)scrollWheel:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::MouseWheel);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setButton(get_mouse_buttons(event));
  ev.setModifiers(get_modifiers_from_nsevent(event));

  int scale = 1;
  if (self.window)
    scale = [(WindowOSXObjc*)self.window scale];

  if (event.hasPreciseScrollingDeltas &&
      // Here we check if this event is coming from a real precise
      // scrolling device like a magic mouse or a touchpad (which use
      // the phase/momentumPhase properties correctly). Some mice,
      // like Logitech MX Master, return the scroll wheel data as
      // preciseScrollingDelta, resulting in unexpected behavior as we
      // don't know how to interpret the scrollingDelta values
      // accurately from those devices.
      (event.phase != NSEventPhaseNone ||
       event.momentumPhase != NSEventPhaseNone)) {
    ev.setPointerType(os::PointerType::Touchpad);
    // TODO we shouldn't change the sign
    ev.setWheelDelta(gfx::Point(-event.scrollingDeltaX / scale,
                                -event.scrollingDeltaY / scale));
    ev.setWheelDeltaF(gfx::PointF(-event.scrollingDeltaX / scale,
                                  -event.scrollingDeltaY / scale));
    ev.setPreciseWheel(true);
  }
  else {
    // Ignore the acceleration factor, just use the wheel sign.
    gfx::Point pt(0, 0);
    if (event.scrollingDeltaX >= 0.1)
      pt.x = -1;
    else if (event.scrollingDeltaX <= -0.1)
      pt.x = 1;
    if (event.scrollingDeltaY >= 0.1)
      pt.y = -1;
    else if (event.scrollingDeltaY <= -0.1)
      pt.y = 1;

    ev.setPointerType(os::PointerType::Mouse);
    ev.setWheelDelta(pt);
    ev.setWheelDeltaF(gfx::PointF(pt.x, pt.y));
  }

  [self queueEvent:ev];
}

- (void)magnifyWithEvent:(NSEvent*)event
{
  Event ev;
  ev.setType(Event::TouchMagnify);
  ev.setMagnification(event.magnification);
  ev.setPosition(get_local_mouse_pos(self, event));
  ev.setModifiers(get_modifiers_from_nsevent(event));
  ev.setPointerType(os::PointerType::Touchpad);
  [self queueEvent:ev];
}

- (void)tabletProximity:(NSEvent*)event
{
  if (event.isEnteringProximity == YES) {
    switch (event.pointingDeviceType) {
      case NSPointingDeviceTypePen: m_pointerType = os::PointerType::Pen; break;
      case NSPointingDeviceTypeCursor: m_pointerType = os::PointerType::Cursor; break;
      case NSPointingDeviceTypeEraser: m_pointerType = os::PointerType::Eraser; break;
      default:
        m_pointerType = os::PointerType::Unknown;
        break;
    }
  }
  else {
    m_pointerType = os::PointerType::Unknown;
  }
}

- (void)cursorUpdate:(NSEvent*)event
{
  [self updateCurrentCursor];
}

- (void)setCursor:(NSCursor*)cursor
{
  m_nsCursor = cursor;
  [self updateCurrentCursor];
}

- (void)createMouseTrackingArea
{
  // Create a tracking area to receive mouseMoved events
  m_trackingArea =
    [[NSTrackingArea alloc]
        initWithRect:self.bounds
             options:(NSTrackingMouseEnteredAndExited |
                      NSTrackingMouseMoved |
                      NSTrackingActiveInActiveApp |
                      NSTrackingEnabledDuringMouseDrag |
                      NSTrackingCursorUpdate)
               owner:self
            userInfo:nil];
  [self addTrackingArea:m_trackingArea];
}

- (void)destroyMouseTrackingArea
{
  if (m_trackingArea) {
    [self removeTrackingArea:m_trackingArea];
    m_trackingArea = nil;
  }
}

- (void)updateCurrentCursor
{
  if (m_nsCursor) {
    if (!m_visibleMouse)
      m_visibleMouse = true;
    [m_nsCursor set];
  }
  else if (m_visibleMouse) {
    m_visibleMouse = false;

    // Instead of using [NSCursor hide], we use a NSCursor with an
    // empty image. This is better because [NSCursor hide/unhide]
    // functions are hard to balance.
    if (!g_emptyNsCursor) {
      NSImage* img = [[NSImage alloc] initWithSize:NSMakeSize(1, 1)];
      g_emptyNsCursor = [[NSCursor alloc] initWithImage:img
                                                hotSpot:NSMakePoint(0, 0)];
    }
    [g_emptyNsCursor set];
  }
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
  return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
  NSPasteboard* pasteboard = [sender draggingPasteboard];

  if ([pasteboard.types containsObject:NSFilenamesPboardType]) {
    NSArray* filenames = [pasteboard propertyListForType:NSFilenamesPboardType];

    os::Event ev = generate_drop_files_from_nsarray(filenames);
    [self queueEvent:ev];
    return YES;
  }
  else
    return NO;
}

- (void)doCommandBySelector:(SEL)selector
{
  // Do nothing (avoid beep pressing Escape key)
}

- (void)setTranslateDeadKeys:(BOOL)state
{
  g_translateDeadKeys = (state ? true: false);
  g_lastDeadKeyState = 0;
}

- (void)queueEvent:(os::Event&)ev
{
  if (m_impl)
    m_impl->queueEvent(ev);
  else
    os::queue_event(ev);
}

@end
