#pragma once

#include "core/game_enums.hpp"
#include <cstdint>

namespace hb {

// Protocol uses 0-7 for directions (0=North, 1=NE, ..., 7=NW)
// Internal enum uses 1-8 (with 0=none)
// These functions convert between the two representations

// Convert protocol direction (0-7) to internal direction enum (1-8)
inline direction direction_from_protocol(int16_t protocol_dir)
{
    if (protocol_dir < 0 || protocol_dir > 7)
    {
        return direction::none;
    }
    // Protocol 0-7 maps to enum 1-8
    return static_cast<direction>(protocol_dir + 1);
}

// Convert internal direction enum (1-8) to protocol direction (0-7)
inline int16_t direction_to_protocol(direction dir)
{
    if (dir == direction::none)
    {
        return 0;  // Default to north if none
    }
    // Enum 1-8 maps to protocol 0-7
    return static_cast<int16_t>(dir) - 1;
}

// Convert direction enum to sprite direction index (1-8)
// Returns default_dir if direction is none or out of valid range
inline int32_t direction_to_sprite_index(direction dir, int32_t default_dir = 5)
{
    int32_t idx = static_cast<int32_t>(dir);
    // Valid sprite direction indices are 1-8
    if (idx < 1 || idx > 8)
    {
        return default_dir;  // Default to south (5) if invalid
    }
    return idx;
}

// Calculate 8-way direction from source to destination
inline direction calculate_direction(int32_t from_x, int32_t from_y,
                                     int32_t to_x, int32_t to_y)
{
    if (from_x == to_x && from_y == to_y)
    {
        return direction::none;
    }

    int32_t dx = to_x - from_x;
    int32_t dy = to_y - from_y;

    // Pure cardinal directions
    if (dx == 0)
    {
        return (dy < 0) ? direction::north : direction::south;
    }
    if (dy == 0)
    {
        return (dx < 0) ? direction::west : direction::east;
    }

    // Diagonal directions
    double slope = static_cast<double>(dy) / static_cast<double>(dx);

    if (slope < -3.0 || slope > 3.0)
    {
        return (dy < 0) ? direction::north : direction::south;
    }
    else if (slope >= -0.3333 && slope <= 0.3333)
    {
        return (dx < 0) ? direction::west : direction::east;
    }
    else if (slope > 0.3333 && slope <= 3.0)
    {
        // Positive slope: dx and dy same sign
        return (dx < 0) ? direction::north_west : direction::south_east;
    }
    else
    {
        // Negative slope: dx and dy opposite signs
        return (dx < 0) ? direction::south_west : direction::north_east;
    }
}

} // namespace hb
