#pragma once

#include <cstdint>

namespace hb {

// Tile dimensions (from original)
inline constexpr int32_t tile_width = 32;
inline constexpr int32_t tile_height = 32;

// Tile flags
enum class tile_flag : uint16_t {
    none = 0,
    walkable = 1 << 0,
    water = 1 << 1,
    lava = 1 << 2,
    ice = 1 << 3,
    swamp = 1 << 4,
    teleport = 1 << 5,
    blocks_sight = 1 << 6,
    blocks_magic = 1 << 7,
    safe_zone = 1 << 8,
    pvp_zone = 1 << 9,
    occupied = 1 << 10,  // Entity standing here
    item_present = 1 << 11,
};

inline tile_flag operator|(tile_flag a, tile_flag b) {
    return static_cast<tile_flag>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline tile_flag operator&(tile_flag a, tile_flag b) {
    return static_cast<tile_flag>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline bool has_flag(tile_flag flags, tile_flag flag) {
    return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(flag)) != 0;
}

// Single map tile
struct tile {
    uint16_t terrain_id = 0;     // Base terrain sprite
    uint16_t object_id = 0;      // Object/decoration sprite
    uint16_t roof_id = 0;        // Roof sprite (for buildings)
    tile_flag flags = tile_flag::walkable;
    uint8_t light_level = 255;   // 0 = dark, 255 = full bright

    bool is_walkable() const { return has_flag(flags, tile_flag::walkable); }
    bool is_water() const { return has_flag(flags, tile_flag::water); }
    bool is_safe_zone() const { return has_flag(flags, tile_flag::safe_zone); }
    bool blocks_sight() const { return has_flag(flags, tile_flag::blocks_sight); }
    bool is_occupied() const { return has_flag(flags, tile_flag::occupied); }
};

// Tile animation data
struct tile_animation {
    uint16_t base_sprite_id;
    uint8_t frame_count;
    uint8_t frame_delay;  // In ticks
    uint8_t current_frame;
};

} // namespace hb
