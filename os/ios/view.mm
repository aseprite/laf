// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#import "os/ios/view.h"
#include "os/ios/window.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/keys.h"
#include "base/ref.h"

namespace {

os::KeyScancode scancode_from_char(unichar ch)
{
  switch (ch) {
    case 'a': case 'A': return os::kKeyA;
    case 'b': case 'B': return os::kKeyB;
    case 'c': case 'C': return os::kKeyC;
    case 'd': case 'D': return os::kKeyD;
    case 'e': case 'E': return os::kKeyE;
    case 'f': case 'F': return os::kKeyF;
    case 'g': case 'G': return os::kKeyG;
    case 'h': case 'H': return os::kKeyH;
    case 'i': case 'I': return os::kKeyI;
    case 'j': case 'J': return os::kKeyJ;
    case 'k': case 'K': return os::kKeyK;
    case 'l': case 'L': return os::kKeyL;
    case 'm': case 'M': return os::kKeyM;
    case 'n': case 'N': return os::kKeyN;
    case 'o': case 'O': return os::kKeyO;
    case 'p': case 'P': return os::kKeyP;
    case 'q': case 'Q': return os::kKeyQ;
    case 'r': case 'R': return os::kKeyR;
    case 's': case 'S': return os::kKeyS;
    case 't': case 'T': return os::kKeyT;
    case 'u': case 'U': return os::kKeyU;
    case 'v': case 'V': return os::kKeyV;
    case 'w': case 'W': return os::kKeyW;
    case 'x': case 'X': return os::kKeyX;
    case 'y': case 'Y': return os::kKeyY;
    case 'z': case 'Z': return os::kKeyZ;
    case '0': return os::kKey0;
    case '1': return os::kKey1;
    case '2': return os::kKey2;
    case '3': return os::kKey3;
    case '4': return os::kKey4;
    case '5': return os::kKey5;
    case '6': return os::kKey6;
    case '7': return os::kKey7;
    case '8': return os::kKey8;
    case '9': return os::kKey9;
    case ' ': return os::kKeySpace;
    case '\n': case '\r': return os::kKeyEnter;
    case '.': return os::kKeyStop;
    case ',': return os::kKeyComma;
    case '-': return os::kKeyMinus;
    case '/': return os::kKeySlash;
    default: return os::kKeyNil;
  }
}

} // anonymous namespace

@implementation AsepriteView {
  CGFloat _lastPinchScale;
  CGPoint _lastPanTranslation;
}

+ (Class)layerClass
{
  return [CALayer class];
}

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self) {
    self.multipleTouchEnabled = YES;
    self.userInteractionEnabled = YES;
    // Use 1.0 to match point-based coordinate system (like macOS)
    self.layer.contentsScale = 1.0;
    self.layer.magnificationFilter = kCAFilterNearest;
    self.layer.contentsGravity = kCAGravityResize;

    // Pinch to zoom
    UIPinchGestureRecognizer* pinch =
      [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinch:)];
    [self addGestureRecognizer:pinch];

    // Two-finger pan to scroll
    UIPanGestureRecognizer* pan =
      [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
    pan.minimumNumberOfTouches = 2;
    pan.maximumNumberOfTouches = 2;
    [self addGestureRecognizer:pan];
  }
  return self;
}

- (void)updateWithCGImage:(CGImageRef)image
{
  if (!image)
    return;

  if ([NSThread isMainThread]) {
    self.layer.contents = (__bridge id)image;
  }
  else {
    CFRetain(image);
    dispatch_async(dispatch_get_main_queue(), ^{
      self.layer.contents = (__bridge id)image;
      CFRelease(image);
    });
  }
}

- (gfx::Point)locationInView:(UITouch*)touch
{
  CGPoint loc = [touch locationInView:self];
  // Divide by window scale (like macOS get_local_mouse_pos)
  int scale = _aseWindow ? _aseWindow->scale() : 1;
  return gfx::Point((int)(loc.x / scale), (int)(loc.y / scale));
}

- (gfx::Point)gestureLocationInView:(UIGestureRecognizer*)gesture
{
  CGPoint loc = [gesture locationInView:self];
  int scale = _aseWindow ? _aseWindow->scale() : 1;
  return gfx::Point((int)(loc.x / scale), (int)(loc.y / scale));
}

- (void)sendMouseEvent:(os::Event::Type)type touch:(UITouch*)touch
{
  if (!_aseWindow)
    return;

  os::Event ev;
  ev.setType(type);
  ev.setWindow(AddRef(_aseWindow));
  ev.setPosition([self locationInView:touch]);
  ev.setButton(os::Event::LeftButton);
  ev.setPointerType(os::PointerType::Touch);

  if (touch.force > 0 && touch.maximumPossibleForce > 0)
    ev.setPressure((float)(touch.force / touch.maximumPossibleForce));
  else
    ev.setPressure(1.0f);

  os::queue_event(ev);
}

#pragma mark - Touch handling

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  for (UITouch* touch in touches) {
    // Send MouseMove first so the UI manager updates mouse_widget
    // (on desktop this happens naturally as the cursor moves)
    [self sendMouseEvent:os::Event::MouseMove touch:touch];
    [self sendMouseEvent:os::Event::MouseDown touch:touch];
    break;
  }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  for (UITouch* touch in touches) {
    [self sendMouseEvent:os::Event::MouseMove touch:touch];
    break;
  }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  for (UITouch* touch in touches) {
    [self sendMouseEvent:os::Event::MouseUp touch:touch];
    break;
  }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  for (UITouch* touch in touches) {
    [self sendMouseEvent:os::Event::MouseUp touch:touch];
    break;
  }
}

#pragma mark - Pinch to zoom

- (void)handlePinch:(UIPinchGestureRecognizer*)gesture
{
  if (!_aseWindow)
    return;

  if (gesture.state == UIGestureRecognizerStateBegan) {
    _lastPinchScale = 1.0;
  }

  CGFloat currentScale = gesture.scale;
  CGFloat delta = currentScale - _lastPinchScale;
  _lastPinchScale = currentScale;

  os::Event ev;
  ev.setType(os::Event::TouchMagnify);
  ev.setWindow(AddRef(_aseWindow));
  ev.setPosition([self gestureLocationInView:gesture]);
  ev.setMagnification((float)delta);
  ev.setPointerType(os::PointerType::Touch);
  os::queue_event(ev);
}

#pragma mark - Two-finger pan to scroll

- (void)handlePan:(UIPanGestureRecognizer*)gesture
{
  if (!_aseWindow)
    return;

  if (gesture.state == UIGestureRecognizerStateBegan) {
    _lastPanTranslation = CGPointZero;
  }

  CGPoint translation = [gesture translationInView:self];
  CGFloat dx = translation.x - _lastPanTranslation.x;
  CGFloat dy = translation.y - _lastPanTranslation.y;
  _lastPanTranslation = translation;

  os::Event ev;
  ev.setType(os::Event::MouseWheel);
  ev.setWindow(AddRef(_aseWindow));
  ev.setPosition([self gestureLocationInView:gesture]);
  ev.setWheelDelta(gfx::Point((int)dx, (int)dy));
  ev.setPreciseWheel(true);
  ev.setPointerType(os::PointerType::Touch);
  ev.setButton(os::Event::NoneButton);
  os::queue_event(ev);
}

#pragma mark - UIKeyInput (software keyboard)

- (BOOL)canBecomeFirstResponder
{
  return _textInputEnabled;
}

- (BOOL)hasText
{
  return YES;
}

- (void)insertText:(NSString*)text
{
  if (!_aseWindow)
    return;

  for (NSUInteger i = 0; i < text.length; ++i) {
    unichar ch = [text characterAtIndex:i];

    os::Event ev;
    ev.setType(os::Event::KeyDown);
    ev.setUnicodeChar(ch);
    ev.setScancode(scancode_from_char(ch));
    ev.setModifiers(os::kKeyNoneModifier);
    ev.setRepeat(0);
    ev.setWindow(AddRef(_aseWindow));
    os::queue_event(ev);

    os::Event evUp;
    evUp.setType(os::Event::KeyUp);
    evUp.setUnicodeChar(ch);
    evUp.setScancode(scancode_from_char(ch));
    evUp.setModifiers(os::kKeyNoneModifier);
    evUp.setRepeat(0);
    evUp.setWindow(AddRef(_aseWindow));
    os::queue_event(evUp);
  }
}

- (void)deleteBackward
{
  if (!_aseWindow)
    return;

  os::Event ev;
  ev.setType(os::Event::KeyDown);
  ev.setUnicodeChar(0);
  ev.setScancode(os::kKeyBackspace);
  ev.setModifiers(os::kKeyNoneModifier);
  ev.setRepeat(0);
  ev.setWindow(AddRef(_aseWindow));
  os::queue_event(ev);

  os::Event evUp;
  evUp.setType(os::Event::KeyUp);
  evUp.setUnicodeChar(0);
  evUp.setScancode(os::kKeyBackspace);
  evUp.setModifiers(os::kKeyNoneModifier);
  evUp.setRepeat(0);
  evUp.setWindow(AddRef(_aseWindow));
  os::queue_event(evUp);
}

@end
