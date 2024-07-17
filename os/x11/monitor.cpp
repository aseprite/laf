#include "monitor.h"
#include "os/x11/x11.h"

#include <X11/extensions/Xrandr.h>

namespace os {

MonitorsX11::MonitorsX11()
{
  auto x11display = X11::instance()->display();

  int numMonitors;
  XRRMonitorInfo* monitors = XRRGetMonitors(x11display, XDefaultRootWindow(x11display), false, &numMonitors);

  m_numMonitors = numMonitors;
  m_monitors = unique_monitors_ptr(monitors);
}

int MonitorsX11::numMonitors() const { return m_numMonitors; }

const XRRMonitorInfo& MonitorsX11::monitor(int monitorNum) const { return m_monitors.get()[monitorNum]; }

} // namespace os

