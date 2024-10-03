#include "os/x11/monitor.h"
#include "gfx/rect.h"
#include "os/ref.h"
#include "os/screen.h"
#include "os/x11/x11.h"
#include "os/x11/screen.h"

#include <climits>
#include <algorithm>
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

const XRRMonitorInfo& MonitorsX11::monitorInfo(int monitorNum) const { return m_monitors.get()[monitorNum]; }

// searches for the monitor containing the most area of the window
ScreenRef MonitorsX11::nearestMonitorOf(const gfx::Rect& frame) const
{
  ScreenRef candidate = nullptr;
  int most_area = INT_MIN;

  for (int nmonitor=0; nmonitor<m_numMonitors; nmonitor++) {
    ScreenRef monitor = os::make_ref<ScreenX11>(nmonitor);

    const gfx::Rect& bounds = monitor->bounds();
    gfx::Rect segment(frame);
    if (segment.x < bounds.x) {
      segment.w = std::max(0, segment.w - (bounds.x-segment.x));
      segment.x = bounds.x;
    }
    if (segment.x2() > bounds.x2()) {
      segment.w = std::max(0, segment.w - (segment.x2()-bounds.x2()));
    }
    if (segment.y < bounds.y) {
      segment.h = std::max(0, segment.h - (bounds.y-segment.y));
      segment.y = bounds.y;
    }
    if (segment.y2() > bounds.y2()) {
      segment.h = std::max(0, segment.h - (segment.y2()-bounds.y2()));
    }

    int area = segment.w*segment.h;
    if (area > most_area) {
      candidate = monitor;
      most_area = area;
    }
  }

  return candidate;
}

} // namespace os

