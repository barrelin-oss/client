#include "platform/monitor.hpp"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

namespace hb {

#ifdef _WIN32

std::optional<monitor_rect> get_primary_monitor()
{
    POINT origin{0, 0};
    HMONITOR hmon = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);

    if (!GetMonitorInfo(hmon, &mi))
    {
        return std::nullopt;
    }

    return monitor_rect{
        .x = mi.rcWork.left,
        .y = mi.rcWork.top,
        .width = mi.rcWork.right - mi.rcWork.left,
        .height = mi.rcWork.bottom - mi.rcWork.top,
    };
}

#else

std::optional<monitor_rect> get_primary_monitor()
{
    Display* display = XOpenDisplay(nullptr);
    if (!display)
    {
        spdlog::warn("Failed to open X display for monitor query");
        return std::nullopt;
    }

    Window root = DefaultRootWindow(display);
    XRRScreenResources* screen = XRRGetScreenResources(display, root);
    if (!screen)
    {
        XCloseDisplay(display);
        return std::nullopt;
    }

    std::optional<monitor_rect> result;
    RROutput primary = XRRGetOutputPrimary(display, root);

    if (primary != None)
    {
        XRROutputInfo* output = XRRGetOutputInfo(display, screen, primary);
        if (output && output->crtc != None)
        {
            XRRCrtcInfo* crtc = XRRGetCrtcInfo(display, screen, output->crtc);
            if (crtc)
            {
                result = monitor_rect{
                    .x = static_cast<int32_t>(crtc->x),
                    .y = static_cast<int32_t>(crtc->y),
                    .width = static_cast<int32_t>(crtc->width),
                    .height = static_cast<int32_t>(crtc->height),
                };
                XRRFreeCrtcInfo(crtc);
            }
        }
        if (output) XRRFreeOutputInfo(output);
    }

    XRRFreeScreenResources(screen);
    XCloseDisplay(display);
    return result;
}

#endif

} // namespace hb
