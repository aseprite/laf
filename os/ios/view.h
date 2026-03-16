// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_IOS_VIEW_H_INCLUDED
#define OS_IOS_VIEW_H_INCLUDED
#pragma once

#ifdef __OBJC__

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

namespace os {
  class WindowIOS;
}

@interface AsepriteView : UIView <UIKeyInput>
@property (nonatomic, assign) os::WindowIOS* aseWindow;
@property (nonatomic, assign) BOOL textInputEnabled;
- (void)updateWithCGImage:(CGImageRef)image;
@end

#endif

#endif
