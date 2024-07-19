#ifndef MONITOR_H
#define MONITOR_H
#pragma once

#include "gfx/fwd.h"
#include "os/screen.h"

#include <X11/extensions/Xrandr.h>
#include <memory>

namespace os {

struct MonitorCloser {
  void operator()(XRRMonitorInfo* monitors) const noexcept { XRRFreeMonitors(monitors); }
};

typedef std::unique_ptr<XRRMonitorInfo, MonitorCloser> unique_monitors_ptr;

class MonitorsX11 {
public:
  MonitorsX11();

  int numMonitors() const;
  const XRRMonitorInfo& monitorInfo(int monitorNum) const;

  ScreenRef nearestMonitorOf(const gfx::Rect& frame) const;

private:
  int m_numMonitors;
  unique_monitors_ptr m_monitors;
};

} // namespace os

#endif //MONITOR_H
