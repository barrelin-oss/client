#pragma once

#include <cstdint>
#include <string>

namespace hb::debug
{

// Bounds structure for positionable elements
struct bounds
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    bool contains(int32_t px, int32_t py) const { return px >= x && px < x + width && py >= y && py < y + height; }
};

// Simple point structure
struct point
{
    int32_t x = 0;
    int32_t y = 0;
};

// Interface for elements that can be positioned via the debug overlay
class positionable
{
public:
    virtual ~positionable() = default;

    // Unique identifier for this positionable (e.g., "login_screen.panel")
    virtual std::string position_id() const = 0;

    // Get current bounds (position + size)
    virtual bounds get_bounds() const = 0;

    // Set new position
    virtual void set_position(int32_t x, int32_t y) = 0;
};

} // namespace hb::debug
