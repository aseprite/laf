// LAF OS Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <Cocoa/Cocoa.h>

#include "os/osx/app.h"

#include "base/debug.h"
#include "base/thread.h"
#include "os/osx/app_delegate.h"

namespace os {

class AppOSX::Impl {
public:
  bool init()
  {
    m_app = [NSApplicationOSX sharedApplication];
    m_appDelegate = [AppDelegateOSX new];

    [m_app setDelegate:m_appDelegate];

    // Don't activate the application ignoring other apps. This is
    // called by OS X when the application is launched by the user
    // from the application bundle. In this way, we can execute
    // aseprite from the command line/bash scripts and the app will
    // not be activated.
    //[m_app activateIgnoringOtherApps:YES];

    return true;
  }

  void setAppMode(AppMode appMode)
  {
    switch (appMode) {
      case AppMode::CLI: [m_app setActivationPolicy:NSApplicationActivationPolicyProhibited]; break;
      case AppMode::GUI: [m_app setActivationPolicy:NSApplicationActivationPolicyRegular]; break;
    }
  }

  bool isHidden() const { return [m_appDelegate isHidden]; }

  void markCliFileAsProcessed(const std::string& fn) { [m_appDelegate markCliFileAsProcessed:fn]; }

  void finishLaunching()
  {
    id runningApp = [NSRunningApplication currentApplication];
    // [m_app run] must be called once, if the app didn't finish launching yet.
    if (![runningApp isFinishedLaunching]) {
      // The run method must be called in GUI mode only, otherwise the
      // [AppDelegateOSX applicationDidFinishLaunching] doesn't get called
      // and [m_app run] ends up blocking the app.
      if ([runningApp activationPolicy] == NSApplicationActivationPolicyRegular) {
        // Note that the [m_app run] call doesn't block because we are calling
        // [NSApp stop] from [AppDelegateOSX applicationDidFinishLaunching]. We only
        // need the application's initialization done inside run to prevent issues
        // such as: https://github.com/aseprite/aseprite/issues/4795
        [m_app run];
      }
      else {
        // The app is running in CLI mode, then we just call finishLaunching.
        [m_app finishLaunching];
      }
    }

    [m_appDelegate resetCliFiles];
  }

  void activateApp() { [m_app activateIgnoringOtherApps:YES]; }

private:
  NSApplication* m_app;
  AppDelegateOSX* m_appDelegate;
};

static AppOSX* g_instance = nullptr;

// static
AppOSX* AppOSX::instance()
{
  return g_instance;
}

AppOSX::AppOSX() : m_impl(new Impl)
{
  ASSERT(!g_instance);
  g_instance = this;
}

AppOSX::~AppOSX()
{
  ASSERT(g_instance == this);
  g_instance = nullptr;
}

bool AppOSX::init()
{
  return m_impl->init();
}

void AppOSX::setAppMode(AppMode appMode)
{
  m_impl->setAppMode(appMode);
}

bool AppOSX::isHidden() const
{
  return m_impl->isHidden();
}

void AppOSX::markCliFileAsProcessed(const std::string& fn)
{
  m_impl->markCliFileAsProcessed(fn);
}

void AppOSX::finishLaunching()
{
  m_impl->finishLaunching();
}

void AppOSX::activateApp()
{
  m_impl->activateApp();
}

} // namespace os
