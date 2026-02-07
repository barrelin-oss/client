#pragma once

#include <cstdint>
#include <optional>

namespace hb {

struct monitor_rect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

// Returns the primary monitor's position and size in virtual desktop coordinates.
std::optional<monitor_rect> get_primary_monitor();

} // namespace hb
