#pragma once

#include "core/game_enums.hpp"
#include <cstdint>
#include <optional>
#include <utility>

namespace hb
{

// Protocol uses 0-7 for directions (0=North, 1=NE, ..., 7=NW)
// Internal enum uses 1-8
// These functions convert between the two representations

// Convert protocol direction (0-7) to internal direction enum (1-8)
// Returns nullopt for out-of-range input
inline std::optional<direction> direction_from_protocol(int16_t protocol_dir)
{
    if (protocol_dir < 0 || protocol_dir > 7)
    {
        return std::nullopt;
    }
    // Protocol 0-7 maps to enum 1-8
    return static_cast<direction>(protocol_dir + 1);
}

// Convert internal direction enum (1-8) to protocol direction (0-7)
inline int16_t direction_to_protocol(direction dir)
{
    // Enum 1-8 maps to protocol 0-7
    return static_cast<int16_t>(dir) - 1;
}

// Convert direction enum to sprite direction index (1-8)
inline int32_t direction_to_sprite_index(direction dir)
{
    return static_cast<int32_t>(dir);
}

// Overload accepting optional direction with a default fallback
inline int32_t direction_to_sprite_index(std::optional<direction> dir, int32_t default_dir = 5)
{
    return dir ? static_cast<int32_t>(*dir) : default_dir;
}

// Calculate 8-way direction from source to destination
// Returns nullopt when from == to
inline std::optional<direction> calculate_direction(int32_t from_x, int32_t from_y, int32_t to_x, int32_t to_y)
{
    if (from_x == to_x && from_y == to_y)
    {
        return std::nullopt;
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

// Get the tile offset {dx, dy} for a direction
// Matches legacy _tmp_cTmpDirX/Y lookup tables
inline std::pair<int32_t, int32_t> direction_offset(direction dir)
{
    switch (dir)
    {
    case direction::north:
        return {0, -1};
    case direction::north_east:
        return {1, -1};
    case direction::east:
        return {1, 0};
    case direction::south_east:
        return {1, 1};
    case direction::south:
        return {0, 1};
    case direction::south_west:
        return {-1, 1};
    case direction::west:
        return {-1, 0};
    case direction::north_west:
        return {-1, -1};
    }
    return {0, 0}; // Unreachable for valid direction values
}

// Rotate direction one step clockwise (e.g. north -> north_east)
inline direction rotate_cw(direction dir)
{
    auto val = static_cast<uint8_t>(dir);
    return static_cast<direction>(val == 8 ? 1 : val + 1);
}

// Rotate direction one step counter-clockwise (e.g. north -> north_west)
inline direction rotate_ccw(direction dir)
{
    auto val = static_cast<uint8_t>(dir);
    return static_cast<direction>(val == 1 ? 8 : val - 1);
}

} // namespace hb
