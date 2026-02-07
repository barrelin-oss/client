#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hb {

struct monitor_rect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

// Detailed information about a connected monitor
struct monitor_info {
    int32_t index = 0;          // 0-based index
    std::string name;           // e.g. "DP-1", "HDMI-0", "\\\\.\\DISPLAY1"
    int32_t x = 0;              // Position in virtual desktop
    int32_t y = 0;
    int32_t width = 0;          // Native resolution
    int32_t height = 0;
    bool primary = false;
};

// Returns the primary monitor's position and size in virtual desktop coordinates.
std::optional<monitor_rect> get_primary_monitor();

// Returns all connected monitors sorted by x position.
std::vector<monitor_info> enumerate_monitors();

} // namespace hb
