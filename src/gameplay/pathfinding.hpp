#pragma once

#include "core/game_enums.hpp"
#include "core/direction_utils.hpp"
#include <cstdint>
#include <optional>

namespace hb
{

// Simple 8-direction calculation toward destination (no obstacle avoidance)
// Equivalent to legacy CMisc::cGetNextMoveDir
// Returns nullopt when from == to
inline std::optional<direction> get_next_move_dir(int32_t from_x, int32_t from_y, int32_t to_x, int32_t to_y)
{
    if (from_x == to_x && from_y == to_y)
    {
        return std::nullopt;
    }

    int32_t dx = to_x - from_x;
    int32_t dy = to_y - from_y;

    // 8-direction movement toward destination
    if (dx > 0 && dy > 0)
        return direction::south_east;
    if (dx > 0 && dy < 0)
        return direction::north_east;
    if (dx < 0 && dy > 0)
        return direction::south_west;
    if (dx < 0 && dy < 0)
        return direction::north_west;
    if (dx > 0)
        return direction::east;
    if (dx < 0)
        return direction::west;
    if (dy > 0)
        return direction::south;
    if (dy < 0)
        return direction::north;

    return std::nullopt;
}

// Obstacle-avoiding pathfinding - ported from legacy CGame::cGetNextMoveDir
//
// Gets the direct direction toward the destination, then searches for a
// walkable tile. First tries a 3-direction cone in the preferred rotation
// (base + 2 adjacent CW or CCW), then falls back to the opposite rotation.
// This ensures we find a path even when obstacles block one side entirely.
// The player_turn flag alternates each step to create a zigzag pattern
// around obstacles instead of always hugging one side.
//
// Parameters:
//   from_x, from_y   - Current tile position
//   to_x, to_y       - Destination tile position
//   player_turn       - Alternating flag (false=CW first, true=CCW first)
//   is_passable       - Predicate: returns true if a tile can be moved to
//
// Returns the best walkable direction, or nullopt if completely blocked.
template<typename Predicate>
std::optional<direction> get_next_walkable_dir(
    int32_t from_x, int32_t from_y, int32_t to_x, int32_t to_y, bool player_turn, Predicate&& is_passable)
{
    if (from_x == to_x && from_y == to_y)
    {
        return std::nullopt;
    }

    auto base_dir = get_next_move_dir(from_x, from_y, to_x, to_y);
    if (!base_dir)
    {
        return std::nullopt;
    }

    // Try 3-direction cone in preferred rotation: base, +1, +2
    for (int i = 0; i < 3; ++i)
    {
        direction candidate = *base_dir;
        for (int j = 0; j < i; ++j)
        {
            candidate = player_turn ? rotate_ccw(candidate) : rotate_cw(candidate);
        }

        auto [dx, dy] = direction_offset(candidate);
        if (is_passable(from_x + dx, from_y + dy))
        {
            return candidate;
        }
    }

    // Preferred rotation fully blocked - try opposite rotation (+1, +2 only,
    // base was already checked above)
    for (int i = 1; i < 3; ++i)
    {
        direction candidate = *base_dir;
        for (int j = 0; j < i; ++j)
        {
            candidate = player_turn ? rotate_cw(candidate) : rotate_ccw(candidate);
        }

        auto [dx, dy] = direction_offset(candidate);
        if (is_passable(from_x + dx, from_y + dy))
        {
            return candidate;
        }
    }

    return std::nullopt;
}

} // namespace hb
