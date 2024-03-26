// LAF OS Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_DND_H_INCLUDED
#define OS_OSX_DND_H_INCLUDED
#pragma once

#ifdef __OBJC__

#include "base/exception.h"
#include "base/fs.h"
#include "base/paths.h"
#include "gfx/point.h"
#include "os/dnd.h"
#include "os/surface.h"
#include "os/surface_format.h"
#include "os/system.h"

#include <Cocoa/Cocoa.h>
#include <memory>

namespace os {
  class DragDataProviderOSX : public DragDataProvider {
  public:
    DragDataProviderOSX(NSPasteboard* pasteboard) : m_pasteboard(pasteboard) {}

  private:
    NSPasteboard* m_pasteboard;

    base::paths getPaths() override {
      base::paths files;

      if ([m_pasteboard.types containsObject:NSFilenamesPboardType]) {
        NSArray* filenames = [m_pasteboard propertyListForType:NSFilenamesPboardType];
        for (int i=0; i<[filenames count]; ++i) {
          NSString* fn = [filenames objectAtIndex: i];

          files.push_back(base::normalize_path([fn UTF8String]));
        }
      }
      return files;
    }

    SurfaceRef getImage() override {
      // The following code was adapted from the clip library:
      // https://github.com/aseprite/clip/blob/835cd0f7e7a964bb969482117856bc56a0ac12bf/clip_osx.mm#L29

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
    }

    bool contains(DragDataItemType type) override {
      for (NSPasteboardType t in m_pasteboard.types) {
        if (type == DragDataItemType::Paths &&
            [t isEqual: NSFilenamesPboardType])
          return true;

        if (type == DragDataItemType::Image &&
            ([t isEqual: NSPasteboardTypeTIFF] ||
             [t isEqual: NSPasteboardTypePNG]))
          return true;
      }
      return false;
    }
  };
} // namespace os

NSDragOperation as_nsdragoperation(const os::DropOperation op)
{
  NSDragOperation nsdop;
  if (op & os::DropOperation::Copy)
    nsdop |= NSDragOperationCopy;

  if (op & os::DropOperation::Move)
    nsdop |= NSDragOperationMove;

  if (op & os::DropOperation::Link)
    nsdop |= NSDragOperationLink;

  return nsdop;
}

os::DropOperation as_dropoperation(const NSDragOperation nsdop)
{
  int op = 0;
  if (nsdop & NSDragOperationCopy)
    op |= os::DropOperation::Copy;

  if (nsdop & NSDragOperationMove)
    op |= os::DropOperation::Move;

  if (nsdop & NSDragOperationLink)
    op |= os::DropOperation::Link;

  return static_cast<os::DropOperation>(op);
}

gfx::Point drag_position(id<NSDraggingInfo> sender)
{
  NSRect contentRect = [sender.draggingDestinationWindow contentRectForFrameRect: sender.draggingDestinationWindow.frame];
  return gfx::Point(
    sender.draggingLocation.x,
    contentRect.size.height - sender.draggingLocation.y);
}

#endif

#endif
