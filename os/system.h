// LAF OS Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SYSTEM_H_INCLUDED
#define OS_SYSTEM_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "os/app_mode.h"
#include "os/capabilities.h"
#include "os/color_space.h"
#include "os/keys.h"
#include "os/ref.h"
#include "os/screen.h"
#include "os/tablet_options.h"
#include "os/window.h"
#include "os/window_spec.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#if CLIP_ENABLE_IMAGE
namespace clip {
class image;
}
#endif

namespace os {

class ColorSpaceConversion;
class EventQueue;
class Font;
class FontManager;
class Logger;
class Menus;
class NativeDialogs;
class Surface;
class System;

using SystemRef = Ref<System>;

// TODO why we just don't return nullptr if the window creation fails?
//      maybe an error handler function?
class WindowCreationException : public std::runtime_error {
public:
  WindowCreationException(const char* msg) throw() : std::runtime_error(msg) {}
};

class System : public RefCount {
protected:
  virtual ~System() {}

public:
  // The app name is used in several places.
  //
  // For X11 it's used for the WM_CLASS name of the main window.
  //
  // For Windows it's used for 1) the main window class name, and
  // 2) to receive DDE messages (WM_DDE_INITIATE) and convert
  // WM_DDE_EXECUTE messages into Event::DropFiles. This allows to
  // the user double-click files in the File Explorer and open the
  // file in a running instance of your app.
  //
  // To receive DDE messages you have to configure the registry in
  // this way (HKCR=HKEY_CLASSES_ROOT):
  //
  //   HKCR\.appfile  (Default)="AppFile"
  //   HKCR\AppFile   (Default)="App File"
  //   HKCR\AppFile\shell\open\command             (Default)="C:\\...\\AppName.EXE"
  //   HKCR\AppFile\shell\open\ddeexec             (Default)="[open(\"%1\")]"
  //   HKCR\AppFile\shell\open\ddeexec\application (Default)="AppName"
  //   HKCR\AppFile\shell\open\ddeexec\topic       (Default)="system"
  //
  // The default value of "HKCR\AppFile\shell\open\ddeexec\application"
  // must match the "appName" given in this function.
  virtual const std::string& appName() const = 0;
  virtual void setAppName(const std::string& appName) = 0;

  // We can use this function to create an application that can run
  // in CLI and GUI mode depending on the given arguments, and in
  // this way avoid to showing the app in the macOS dock bar if we
  // are running in CLI only.
  virtual void setAppMode(AppMode appMode) = 0;

  // Marks a specific file as a file that was processed in the CLI.
  // Only useful on macOS to avoid generating DropFiles events for
  // files that were processed from the CLI arguments directly.
  virtual void markCliFileAsProcessed(const std::string& cliFile) = 0;

  // On macOS it calls [NSApplication run], which in turn does two things:
  // - It calls [NSApplication finishLaunching], which will produce some extra
  // events like [NSApplicationDelegate application:openFiles:] which generates
  // os::Event::DropFiles events for each file specified in the command line.
  // - It initializes internal state of the NSApplication instance to make it
  // behave as expected under some circumstances.
  //
  // About the first point: you can ignore those DropFiles events if you've
  // already processed through the CLI arguments (app_main(argc, argv)) or
  // you can use markCliFileAsProcessed() before calling this function.
  virtual void finishLaunching() = 0;

  // We might need to call this function when the app is launched
  // from Steam. It appears that there is a bug on macOS Steam
  // client where the app is launched, activated, and then the Steam
  // client is activated again.
  virtual void activateApp() = 0;

  virtual Capabilities capabilities() const = 0;
  bool hasCapability(Capabilities c) const { return (int(capabilities()) & int(c)) == int(c); }

  // Sets the specific API/Options to use to process
  // tablet/stylus/pen messages.
  //
  // It can be used to avoid loading wintab32.dll too (sometimes a
  // program can crash when we load a buggy wintab32.dll, so we need
  // a way to opt-out loading this library.)
  virtual void setTabletOptions(const TabletOptions& opts) = 0;
  virtual TabletOptions tabletOptions() const = 0;

  // Sub-interfaces
  virtual Logger* logger() = 0;
  virtual Menus* menus() = 0;
  virtual NativeDialogs* nativeDialogs() = 0;
  virtual EventQueue* eventQueue() = 0;

  // Returns the main screen
  virtual ScreenRef mainScreen() = 0;

  // Returns a list of screens attached to the computer.
  virtual void listScreens(ScreenList& screens) = 0;

  virtual Window* defaultWindow() = 0;

  // Creates a new window in the operating system with the given
  // specs (width, height, etc.).
  virtual WindowRef makeWindow(const WindowSpec& spec) = 0;

  WindowRef makeWindow(const int contentWidth, const int contentHeight, const int scale = 1)
  {
    return makeWindow(WindowSpec(contentWidth, contentHeight, scale));
  }

  virtual Ref<Surface> makeSurface(int width,
                                   int height,
                                   const os::ColorSpaceRef& colorSpace = nullptr) = 0;
#if CLIP_ENABLE_IMAGE
  virtual Ref<Surface> makeSurface(const clip::image& image) = 0;
#endif
  virtual Ref<Surface> makeRgbaSurface(int width,
                                       int height,
                                       const os::ColorSpaceRef& colorSpace = nullptr) = 0;
  virtual Ref<Surface> loadSurface(const char* filename) = 0;
  virtual Ref<Surface> loadRgbaSurface(const char* filename) = 0;

  // Creates a new cursor with the given surface.
  //
  // Warning: On Windows there is a limit of 10,000 GDI objects per
  // process and creating a cursor needs 3 GDI objects (a HCURSOR
  // and two HBITMAPs).
  virtual Ref<Cursor> makeCursor(const Surface* surface,
                                 const gfx::Point& focus,
                                 const int scale) = 0;

  // New font manager
  virtual FontManager* fontManager() = 0;

  // Old font functions (to be removed)
  virtual Ref<Font> loadSpriteSheetFont(const char* filename, int scale = 1) = 0;
  virtual Ref<Font> loadTrueTypeFont(const char* filename, int height) = 0;

  // Returns true if the the given scancode key is pressed/actived.
  virtual bool isKeyPressed(KeyScancode scancode) = 0;

  // Returns the active pressed modifiers.
  virtual KeyModifiers keyModifiers() = 0;

  // Returns the latest unicode character that activated the given
  // scancode.
  virtual int getUnicodeFromScancode(KeyScancode scancode) = 0;

  // Indicates if you want to use dead keys or not. By default it's
  // false, which behaves as regular shortcuts. You should set this
  // to true when you're inside a text field in your app.
  //
  // TODO Improve this API using different input modes,
  //      e.g. GameLike, TextInput, TextInputWithDeadKeys
  virtual void setTranslateDeadKeys(bool state) = 0;

  // Returns the mouse position in the screen. Try to avoid using
  // this and prefer the Event mouse position.
  virtual gfx::Point mousePosition() const = 0;

  // Sets the mouse position to a specific point in the screen.
  virtual void setMousePosition(const gfx::Point& screenPosition) = 0;

  // Gets a color from the desktop in given screen position.
  //
  // WARNING for macOS: This function will ask the user for
  // permissions to record the screen. If the app is not in a signed
  // bundle, the color will be from the wallpaper, but if the
  // function is used in a signed bundled app, the color will be
  // from any opened window.
  virtual gfx::Color getColorFromScreen(const gfx::Point& screenPosition) const = 0;

  // Color management
  virtual void listColorSpaces(std::vector<os::ColorSpaceRef>& list) = 0;
  virtual os::ColorSpaceRef makeColorSpace(const gfx::ColorSpaceRef& colorSpace) = 0;
  virtual Ref<ColorSpaceConversion> convertBetweenColorSpace(const os::ColorSpaceRef& src,
                                                             const os::ColorSpaceRef& dst) = 0;

  // Set a default color profile for all windows (nullptr to use the
  // active monitor color profile and change it dynamically when the
  // window changes to another monitor).
  virtual void setWindowsColorSpace(const os::ColorSpaceRef& cs) = 0;
  virtual os::ColorSpaceRef windowsColorSpace() = 0;

  // Function called to handle a "live resize"/resizing loop of a
  // native window. If this is nullptr, an Event::ResizeWindow is
  // generated when the resizing is finished.
  //
  // TODO I think we should have a SystemDelegate or something
  //      similar instead of a public property.
  std::function<void(os::Window*)> handleWindowResize = nullptr;

#if LAF_WINDOWS
  // Only useful on Windows, the delegate must be a pointer to a
  // WintabAPI::Delegate, and it must be deleted by the user
  // manually (it's not owned by the os::System impl).
  //
  // This can be used to get information about the wintab32.dll
  // vendor (company name, etc.)
  virtual void setWintabDelegate(void* delegate) {}
#endif
};

SystemRef make_system();
System* instance();
void set_instance(System* system);

} // namespace os

#endif
