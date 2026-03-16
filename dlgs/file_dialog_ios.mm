// laf-dlgs
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "dlgs/file_dialog.h"
#include "base/fs.h"

@interface FileDialogHelper : NSObject <UIDocumentPickerDelegate>
@property (nonatomic, assign) BOOL completed;
@property (nonatomic, assign) BOOL cancelled;
@property (nonatomic, strong) NSMutableArray<NSURL*>* selectedURLs;
@property (nonatomic, strong) NSCondition* condition;
@end

@implementation FileDialogHelper

- (instancetype)init
{
  self = [super init];
  if (self) {
    _completed = NO;
    _cancelled = NO;
    _selectedURLs = [NSMutableArray new];
    _condition = [NSCondition new];
  }
  return self;
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller
  didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
  [_condition lock];
  [_selectedURLs addObjectsFromArray:urls];
  _completed = YES;
  [_condition signal];
  [_condition unlock];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
  [_condition lock];
  _cancelled = YES;
  _completed = YES;
  [_condition signal];
  [_condition unlock];
}

@end

namespace {

UIViewController* getRootViewController()
{
  UIWindow* keyWindow = nil;
  for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
    if ([scene isKindOfClass:[UIWindowScene class]]) {
      UIWindowScene* windowScene = (UIWindowScene*)scene;
      for (UIWindow* w in windowScene.windows) {
        if (w.isKeyWindow) {
          keyWindow = w;
          break;
        }
      }
    }
  }
  return keyWindow.rootViewController;
}

} // anonymous namespace

namespace dlgs {

class FileDialogIOS : public FileDialog {
public:
  FileDialogIOS(const Spec& spec) {}

  std::string fileName() override { return m_filename; }

  void getMultipleFileNames(base::paths& output) override { output = m_filenames; }

  void setFileName(const std::string& filename) override { m_filename = filename; }

  Result show(void* windowNative) override
  {
    __block FileDialogHelper* helper = [[FileDialogHelper alloc] init];

    // For Save: Aseprite wants a file path to write to.
    // Strategy: save to a temp file in our sandbox, then present
    // the document picker to export it to the user's chosen location.
    // But Aseprite calls the dialog BEFORE writing, so we need to:
    // 1. Let the user pick a destination folder
    // 2. Return folder + filename so Aseprite writes there
    // 3. Use security-scoped bookmarks to keep access

    if (m_type == Type::SaveFile) {
      return showSaveDialog(helper);
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      @autoreleasepool {
        NSMutableArray<UTType*>* contentTypes = [NSMutableArray new];

        for (const auto& filter : m_filters) {
          NSString* ext = [NSString stringWithUTF8String:filter.first.c_str()];
          UTType* type = [UTType typeWithFilenameExtension:ext];
          if (type)
            [contentTypes addObject:type];
        }

        if (contentTypes.count == 0) {
          [contentTypes addObject:UTTypeImage];
          [contentTypes addObject:UTTypeData];
        }

        UIDocumentPickerViewController* picker = nil;

        if (m_type == Type::OpenFolder) {
          picker = [[UIDocumentPickerViewController alloc]
            initForOpeningContentTypes:@[UTTypeFolder]];
        }
        else {
          picker = [[UIDocumentPickerViewController alloc]
            initForOpeningContentTypes:contentTypes];
          picker.allowsMultipleSelection = (m_type == Type::OpenFiles);
        }

        picker.delegate = helper;
        picker.modalPresentationStyle = UIModalPresentationFormSheet;

        UIViewController* rootVC = getRootViewController();
        if (rootVC) {
          [rootVC presentViewController:picker animated:YES completion:nil];
        }
        else {
          [helper.condition lock];
          helper.cancelled = YES;
          helper.completed = YES;
          [helper.condition signal];
          [helper.condition unlock];
        }
      }
    });

    [helper.condition lock];
    while (!helper.completed) {
      [helper.condition wait];
    }
    [helper.condition unlock];

    if (helper.cancelled || helper.selectedURLs.count == 0) {
      return Result::Cancel;
    }

    m_filenames.clear();
    for (NSURL* url in helper.selectedURLs) {
      [url startAccessingSecurityScopedResource];
      m_filenames.push_back([[url path] UTF8String]);
    }

    if (!m_filenames.empty())
      m_filename = m_filenames.front();

    return Result::OK;
  }

private:
  Result showSaveDialog(FileDialogHelper* helper)
  {
    // Extract the filename from m_filename (e.g. "sprite.png")
    std::string baseName = base::get_file_name(m_filename);
    if (baseName.empty())
      baseName = "export.png";

    // Save to a temp file in the app sandbox first, then export
    // We'll write to Documents/<filename>, then present the export picker
    NSString* tempDir = NSTemporaryDirectory();
    NSString* tempFile = [tempDir stringByAppendingPathComponent:
      [NSString stringWithUTF8String:baseName.c_str()]];
    std::string tempPath = [tempFile UTF8String];

    // Return the temp path so Aseprite writes the file there
    // After Aseprite writes, it will call invalidateRegion which
    // triggers a redraw. But we need a two-step process:
    // Aseprite expects the dialog to return a path, writes to it,
    // then is done. So we return a writable path in our sandbox.
    m_filename = tempPath;
    m_filenames.clear();
    m_filenames.push_back(tempPath);

    // After returning, Aseprite will write to tempPath.
    // We need to present the share/export sheet AFTER the write completes.
    // Schedule it to run after a short delay on the main thread.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)),
      dispatch_get_main_queue(), ^{
        NSURL* fileURL = [NSURL fileURLWithPath:tempFile];
        if (![[NSFileManager defaultManager] fileExistsAtPath:tempFile])
          return;

        UIViewController* rootVC = getRootViewController();
        if (!rootVC)
          return;

        // Present a share sheet so the user can save to Files, share, etc.
        UIActivityViewController* activityVC =
          [[UIActivityViewController alloc]
            initWithActivityItems:@[fileURL]
            applicationActivities:nil];

        // On iPad, must present as popover
        if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
          activityVC.popoverPresentationController.sourceView = rootVC.view;
          activityVC.popoverPresentationController.sourceRect =
            CGRectMake(rootVC.view.bounds.size.width / 2,
                       rootVC.view.bounds.size.height / 2, 1, 1);
        }

        [rootVC presentViewController:activityVC animated:YES completion:nil];
      });

    return Result::OK;
  }

  std::string m_filename;
  base::paths m_filenames;
};

FileDialogRef FileDialog::makeIOS(const Spec& spec)
{
  return base::make_ref<FileDialogIOS>(spec);
}

} // namespace dlgs
