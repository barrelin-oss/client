#pragma once

#include "core/game_enums.hpp"
#include "world/map.hpp"
#include <cstdint>

namespace hb {

// Legacy Helbreath pathfinding - calculates next move direction toward destination
// Simple direct movement toward target, no obstacle avoidance
inline direction get_next_move_dir(int32_t from_x, int32_t from_y,
                                   int32_t to_x, int32_t to_y)
{
    // Already at destination
    if (from_x == to_x && from_y == to_y) {
        return direction::none;
    }

    int32_t dx = to_x - from_x;
    int32_t dy = to_y - from_y;

    // Simple 8-direction movement - move toward destination
    // Prefer diagonal when both dx and dy are non-zero
    if (dx > 0 && dy > 0) return direction::south_east;
    if (dx > 0 && dy < 0) return direction::north_east;
    if (dx < 0 && dy > 0) return direction::south_west;
    if (dx < 0 && dy < 0) return direction::north_west;
    if (dx > 0) return direction::east;
    if (dx < 0) return direction::west;
    if (dy > 0) return direction::south;
    if (dy < 0) return direction::north;

    return direction::none;
}

} // namespace hb
