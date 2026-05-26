// laf-dlgs
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <Cocoa/Cocoa.h>
#include <vector>

#include "dlgs/file_dialog.h"

#include "base/fs.h"

#include <map>

namespace {

// macOS does a super strange thing to handle the keyboard shortcuts
// when a NSSavePanel is open: it sends all the Command+key
// combinations to the main menu to handle the most basic shortcuts
// like Command+C (to run the copy: selector), or Command+V (paste:),
// etc.
//
// So what we have to do if the main menu doesn't provide the standard
// "Edit" selectors? Which can be our case with laf-os MenuItemOSX and
// MenuOSX implementations:
//
// 1. Before we open the NSSavePanel, this class receives the current
//    "Edit" menu that was specified by the user from
//    FileDialog::Spec::editNSMenuItem.
// 2. Replaces the "Edit" menu with a custom made one with the
//    standard Edit menu prepared especially with selectors (undo:,
//    redo:, cut:, etc.)
// 3. Each menu item that contains a standard keyboard shortcut
//    (Command+C, Command+V, etc.) must be modified, because those
//    shortcuts are now used by this new Edit menu. So we remove the
//    keyEquivalent of each one. This is necessary only when one of
//    those items are outside the replaced Edit menu (e.g. if we have
//    Command+A to select all in other menu like Select > All, instead
//    of Edit > Select All)
// 4. After the NSSavePanel is closed, it restores all keyEquivalent
//    shortcuts and the old Edit menu.
//
class OSXEditMenuHack {
public:
  OSXEditMenuHack(NSMenuItem* currentEditMenuItem) : m_editMenuItem(currentEditMenuItem)
  {
    if (!m_editMenuItem)
      return;

    auto editMenu = [NSMenu new];

    // Check MenuItemOSX::syncTitle(), but basically on macOS the
    // NSMenu title is the one displayed for NSMenuItem with submenus.
    editMenu.title = currentEditMenuItem.title;

    [editMenu addItem:newItem(@"Undo", @selector(undo:), @"z", NSEventModifierFlagCommand)];
    [editMenu addItem:newItem(@"Redo", @selector(redo:), @"Z", NSEventModifierFlagCommand)];
    [editMenu addItem:[NSMenuItem separatorItem]];
    [editMenu addItem:newItem(@"Cut", @selector(cut:), @"x", NSEventModifierFlagCommand)];
    [editMenu addItem:newItem(@"Copy", @selector(copy:), @"c", NSEventModifierFlagCommand)];
    [editMenu addItem:newItem(@"Paste", @selector(paste:), @"v", NSEventModifierFlagCommand)];
    [editMenu addItem:newItem(@"Delete", @selector(delete:), @"", 0)];
    [editMenu
      addItem:newItem(@"Select All", @selector(selectAll:), @"a", NSEventModifierFlagCommand)];

    m_submenu = m_editMenuItem.submenu;
    m_editMenuItem.submenu = editMenu;
  }

  ~OSXEditMenuHack()
  {
    if (!m_editMenuItem)
      return;

    // Restore all key equivalents
    for (auto& kv : m_restoreKeys)
      kv.first.keyEquivalent = kv.second;

    m_editMenuItem.submenu = m_submenu;
  }

private:
  NSMenuItem* newItem(NSString* title, SEL sel, NSString* key, NSEventModifierFlags flags)
  {
    auto item = [[NSMenuItem alloc] initWithTitle:title action:sel keyEquivalent:key];
    if (flags)
      item.keyEquivalentModifierMask = flags;

    // Search for menu items in the main menu that already contain the
    // same keyEquivalent, and disable them temporarily.
    disableMenuItemsWithKeyEquivalent([[NSApplication sharedApplication] mainMenu], key, flags);

    return item;
  }

  void disableMenuItemsWithKeyEquivalent(NSMenu* menu, NSString* key, NSEventModifierFlags flags)
  {
    for (NSMenuItem* item in menu.itemArray) {
      if ([item.keyEquivalent isEqualToString:key] && item.keyEquivalentModifierMask == flags) {
        m_restoreKeys[item] = item.keyEquivalent;
        item.keyEquivalent = @"";
      }
      if (item.submenu)
        disableMenuItemsWithKeyEquivalent(item.submenu, key, flags);
    }
  }

  NSMenuItem* m_editMenuItem = nullptr;
  NSMenu* m_submenu = nullptr;
  std::map<NSMenuItem*, NSString*> m_restoreKeys;
};

} // anonymous namespace

@interface OpenSaveHelper : NSObject {
@private
  NSSavePanel* panel;
  NSWindow* window;
  int result;
  std::function<void()> fileTypeChangeObserver;
}
- (id)init;
- (void)setPanel:(NSSavePanel*)panel;
- (void)setWindow:(NSWindow*)window;
- (void)runModal;
- (int)result;
- (void)fileTypeChange;
- (void)setFileTypeChangeObserver:(std::function<void()>)observer;
@end

@implementation OpenSaveHelper

- (id)init
{
  if (self = [super init]) {
    panel = nil;
    window = nil;
    result = NSFileHandlingPanelCancelButton;
  }
  return self;
}

- (void)setPanel:(NSSavePanel*)newPanel
{
  panel = newPanel;
}

- (void)setWindow:(NSWindow*)newWindow
{
  window = newWindow;
}

// This is executed in the main thread.
- (void)runModal
{
  [[[NSApplication sharedApplication] mainMenu] setAutoenablesItems:NO];

#ifndef __MAC_10_6 // runModalForTypes is deprecated in 10.6
  if ([panel isKindOfClass:[NSOpenPanel class]]) {
    // As we're using OS X 10.4 framework, it looks like runModal
    // doesn't recognize the allowedFileTypes property. So we force it
    // using runModalForTypes: selector.

    result = [(NSOpenPanel*)panel runModalForTypes:[panel allowedFileTypes]];
  }
  else
#endif
  {
    result = [panel runModal];
  }

  [[[NSApplication sharedApplication] mainMenu] setAutoenablesItems:YES];

  // Re-active the parent key window.
  if (window)
    [window makeKeyAndOrderFront:nil];
}

- (int)result
{
  return result;
}

- (void)fileTypeChange
{
  if (fileTypeChangeObserver)
    fileTypeChangeObserver();
}

- (void)setFileTypeChangeObserver:(std::function<void()>)observer
{
  fileTypeChangeObserver = observer;
}

@end

namespace dlgs {

class FileDialogOSX : public FileDialog {
public:
  FileDialogOSX(const Spec& spec)
  {
    if (spec.editNSMenuItem)
      m_editMenuItem = (__bridge NSMenuItem*)spec.editNSMenuItem;
  }

  std::string fileName() override { return m_filename; }

  void getMultipleFileNames(base::paths& output) override { output = m_filenames; }

  void setFileName(const std::string& filename) override { m_filename = filename; }

  Result show(void* window) override
  {
    Result retValue = Result::Cancel;
    @autoreleasepool {
      NSSavePanel* panel = nil;

      if (m_type == Type::SaveFile) {
        panel = [NSSavePanel new];
      }
      else {
        panel = [NSOpenPanel new];
        [(NSOpenPanel*)panel setAllowsMultipleSelection:(m_type == Type::OpenFiles ? YES : NO)];
        [(NSOpenPanel*)panel setCanChooseFiles:(m_type != Type::OpenFolder ? YES : NO)];
        [(NSOpenPanel*)panel setCanChooseDirectories:(m_type == Type::OpenFolder ? YES : NO)];
      }

      [panel setTitle:[NSString stringWithUTF8String:m_title.c_str()]];
      [panel setCanCreateDirectories:YES];

      if (m_type != Type::OpenFolder && !m_filters.empty()) {
        NSMutableArray* types = [[NSMutableArray alloc] init];

        if (m_type == Type::OpenFile || m_type == Type::OpenFiles) {
          // Empty item so we can show all the allowed file types again
          [types addObject:@""];
        }
        else if (m_type == Type::SaveFile) {
          // The first extension in the array is used as the default one.
          if (!m_defExtension.empty())
            [types addObject:[NSString stringWithUTF8String:m_defExtension.c_str()]];
        }

        for (const auto& filter : m_filters)
          [types addObject:[NSString stringWithUTF8String:filter.first.c_str()]];

        [panel setAllowedFileTypes:types];
        if (m_type == Type::SaveFile)
          [panel setAllowsOtherFileTypes:NO];

        // Create accessory view to select file type
        [panel setAccessoryView:createAccessoryView(panel)];
      }

      // Always show the extension
      [panel setExtensionHidden:NO];

      std::string defPath = m_type == Type::OpenFolder ? m_filename :
                                                         base::get_file_path(m_filename);
      std::string defName = base::get_file_name(m_filename);
      if (!defPath.empty())
        [panel
          setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:defPath.c_str()]]];
      if (!defName.empty())
        [panel setNameFieldStringValue:[NSString stringWithUTF8String:defName.c_str()]];

      OpenSaveHelper* helper = [OpenSaveHelper new];
      [helper setPanel:panel];
      if (window)
        [helper setWindow:(__bridge NSWindow*)window];

      // Configure the file type popup/combobox.
      if (m_popup) {
        [m_popup setTarget:helper];
        [m_popup setAction:@selector(fileTypeChange)];

        // Select the file type item depending on the extension of
        // m_filename.
        if (!m_filename.empty()) {
          auto ext = base::get_file_extension(m_filename);
          if (ext.empty())
            ext = m_defExtension;
          int i = 0;
          for (NSMenuItem* item in m_popup.itemArray) {
            if (ext == item.title.UTF8String) {
              [m_popup selectItemAtIndex:i];
              break;
            }
            ++i;
          }
        }

        // Function to be called when user selects another file type
        // from the combobox.
        [helper setFileTypeChangeObserver:[this, panel]() {
          NSString* extension = m_popup.selectedItem.title;
          if (extension.length == 0) {
            NSMutableArray* allTypes = [[NSMutableArray alloc] init];
            for (NSMenuItem* item in m_popup.itemArray) {
              if (item.title.length != 0)
                [allTypes addObject:item.title];
            }
            [panel setAllowedFileTypes:allTypes];
          }
          else {
            [panel setAllowedFileTypes:@[extension]];
          }
        }];
      }

      {
        OSXEditMenuHack hack(m_editMenuItem);
        [helper performSelectorOnMainThread:@selector(runModal) withObject:nil waitUntilDone:YES];
      }

      if ([helper result] == NSFileHandlingPanelOKButton) {
        if (m_type == Type::OpenFiles) {
          for (NSURL* url in [(NSOpenPanel*)panel URLs]) {
            m_filename = [[url path] UTF8String];
            m_filenames.push_back(m_filename);
          }
        }
        else {
          NSURL* url = [panel URL];
          m_filename = [[url path] UTF8String];
          m_filenames.push_back(m_filename);
        }
        retValue = Result::OK;
      }

      m_popup = nullptr;
    }
    return retValue;
  }

private:
  // The accessory view is any extra NSView that we want to show in
  // the NSSavePanel/NSOpenPanel, in our case, we want to show a
  // combobox (NSPopUpButton) with the available file formats.
  NSView* createAccessoryView(NSSavePanel* panel)
  {
    auto label = [NSTextField labelWithString:@"File Format:"];

    if ([NSFont respondsToSelector:@selector(smallSystemFontSize)])
      label.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    if ([NSColor respondsToSelector:@selector(secondaryLabelColor)])
      label.textColor = [NSColor secondaryLabelColor];

    // Combobox with file formats
    auto popup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:false];
    popup.autoenablesItems = false; // All items enabled
    for (NSString* item in panel.allowedFileTypes) {
      [popup addItemWithTitle:item];
    }
    m_popup = popup;

    auto hbox = [[NSStackView alloc] initWithFrame:NSZeroRect];
    [hbox addView:label inGravity:NSStackViewGravityCenter];
    [hbox addView:popup inGravity:NSStackViewGravityCenter];

    auto accessoryView = [[NSStackView alloc] initWithFrame:NSZeroRect];
    [accessoryView addView:hbox inGravity:NSStackViewGravityCenter];
    accessoryView.edgeInsets = NSEdgeInsetsMake(10, 0, 10, 0);

    return accessoryView;
  }

  std::string m_filename;
  base::paths m_filenames;

  // Keeps a pointer to the list/combobox of file types so we use it
  // easily (configuring its target/action properties + accessing its
  // selected item).
  NSPopUpButton* m_popup = nullptr;

  // Pointer to the "Edit" menu used with OSXEditMenuHack.
  NSMenuItem* m_editMenuItem = nullptr;
};

FileDialogRef FileDialog::makeOSX(const Spec& spec)
{
  return base::make_ref<FileDialogOSX>(spec);
}

} // namespace dlgs
