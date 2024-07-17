#ifndef MONITOR_H
#define MONITOR_H
#pragma once

#include <memory>
#include <X11/extensions/Xrandr.h>

namespace os {

struct MonitorCloser {
  void operator()(XRRMonitorInfo* monitors) const noexcept { XRRFreeMonitors(monitors); }
};

typedef std::unique_ptr<XRRMonitorInfo, MonitorCloser> unique_monitors_ptr;

class MonitorsX11 {
public:
  MonitorsX11();
  int numMonitors() const;
  const XRRMonitorInfo& monitor(int monitorNum) const;

private:
  int m_numMonitors;
  unique_monitors_ptr m_monitors;
};

} // namespace os

#endif //MONITOR_H
