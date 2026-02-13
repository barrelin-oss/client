#include "platform/monitor.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
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

namespace {

struct enum_context {
    std::vector<monitor_info> monitors;
};

BOOL CALLBACK monitor_enum_proc(HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
{
    auto* ctx = reinterpret_cast<enum_context*>(lparam);

    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hmon, &mi))
    {
        return TRUE;  // Continue enumeration
    }

    monitor_info info;
    info.x = mi.rcMonitor.left;
    info.y = mi.rcMonitor.top;
    info.width = mi.rcMonitor.right - mi.rcMonitor.left;
    info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    info.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;

    // Convert device name from wide string
    int len = WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, nullptr, 0, nullptr, nullptr);
    if (len > 0)
    {
        std::string name(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, name.data(), len, nullptr, nullptr);
        info.name = std::move(name);
    }

    ctx->monitors.push_back(std::move(info));
    return TRUE;
}

} // anonymous namespace

std::vector<monitor_info> enumerate_monitors()
{
    enum_context ctx;
    EnumDisplayMonitors(nullptr, nullptr, monitor_enum_proc, reinterpret_cast<LPARAM>(&ctx));

    // Sort by x position for spatial consistency
    std::sort(ctx.monitors.begin(), ctx.monitors.end(),
        [](const monitor_info& a, const monitor_info& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.y < b.y;
        });

    // Assign indices after sorting
    for (size_t i = 0; i < ctx.monitors.size(); ++i)
    {
        ctx.monitors[i].index = static_cast<int32_t>(i);
    }

    spdlog::debug("Enumerated {} monitors", ctx.monitors.size());
    return ctx.monitors;
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

std::vector<monitor_info> enumerate_monitors()
{
    std::vector<monitor_info> monitors;

    Display* display = XOpenDisplay(nullptr);
    if (!display)
    {
        spdlog::warn("Failed to open X display for monitor enumeration");
        return monitors;
    }

    Window root = DefaultRootWindow(display);
    XRRScreenResources* screen = XRRGetScreenResources(display, root);
    if (!screen)
    {
        XCloseDisplay(display);
        return monitors;
    }

    RROutput primary_output = XRRGetOutputPrimary(display, root);

    for (int i = 0; i < screen->noutput; ++i)
    {
        XRROutputInfo* output = XRRGetOutputInfo(display, screen, screen->outputs[i]);
        if (!output) continue;

        // Only include connected outputs with active CRTCs
        if (output->connection == RR_Connected && output->crtc != None)
        {
            XRRCrtcInfo* crtc = XRRGetCrtcInfo(display, screen, output->crtc);
            if (crtc)
            {
                monitor_info info;
                info.name = output->name ? std::string(output->name, output->nameLen) : "Unknown";
                info.x = static_cast<int32_t>(crtc->x);
                info.y = static_cast<int32_t>(crtc->y);
                info.width = static_cast<int32_t>(crtc->width);
                info.height = static_cast<int32_t>(crtc->height);
                info.primary = (screen->outputs[i] == primary_output);

                monitors.push_back(std::move(info));
                XRRFreeCrtcInfo(crtc);
            }
        }

        XRRFreeOutputInfo(output);
    }

    XRRFreeScreenResources(screen);
    XCloseDisplay(display);

    // Sort by x position for spatial consistency
    std::sort(monitors.begin(), monitors.end(),
        [](const monitor_info& a, const monitor_info& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.y < b.y;
        });

    // Assign indices after sorting
    for (size_t i = 0; i < monitors.size(); ++i)
    {
        monitors[i].index = static_cast<int32_t>(i);
    }

    spdlog::debug("Enumerated {} monitors", monitors.size());
    return monitors;
}

#endif

} // namespace hb
