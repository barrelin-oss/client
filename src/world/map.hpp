#pragma once

#include "world/tile.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace hb {

// Map dimensions (from original)
inline constexpr int32_t max_map_width = 752;
inline constexpr int32_t max_map_height = 752;

// Visible area
inline constexpr int32_t visible_tiles_x = 21;  // ~640/32 + margin
inline constexpr int32_t visible_tiles_y = 16;  // ~480/32 + margin

// Map data structure
class map {
public:
    map() = default;
    ~map() = default;

    map(const map&) = delete;
    map& operator=(const map&) = delete;
    map(map&&) noexcept = default;
    map& operator=(map&&) noexcept = default;

    // Load map from file
    bool load(std::string_view path);
    bool load_from_memory(const uint8_t* data, size_t size);
    void clear();

    // Map info
    std::string_view name() const { return name_; }
    void set_name(std::string_view name) { name_ = name; }
    int32_t width() const { return width_; }
    int32_t height() const { return height_; }
    bool is_loaded() const { return !tiles_.empty(); }

    // Tile access
    const tile& get_tile(int32_t x, int32_t y) const;
    tile& get_tile_mut(int32_t x, int32_t y);
    bool is_valid_position(int32_t x, int32_t y) const;

    // Update tile state
    void set_tile_flag(int32_t x, int32_t y, tile_flag flag, bool set);

    // Coordinate conversion
    static int32_t world_to_tile_x(int32_t world_x) { return world_x / tile_width; }
    static int32_t world_to_tile_y(int32_t world_y) { return world_y / tile_height; }
    static int32_t tile_to_world_x(int32_t tile_x) { return tile_x * tile_width; }
    static int32_t tile_to_world_y(int32_t tile_y) { return tile_y * tile_height; }

    // Pathfinding helpers
    bool is_walkable(int32_t x, int32_t y) const;
    bool has_line_of_sight(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const;

    // Find adjacent walkable tile
    std::optional<std::pair<int32_t, int32_t>> find_nearest_walkable(int32_t x, int32_t y, int32_t range = 3) const;

private:
    std::string name_;
    int32_t width_ = 0;
    int32_t height_ = 0;
    std::vector<tile> tiles_;

    static const tile null_tile_;

    size_t tile_index(int32_t x, int32_t y) const {
        return static_cast<size_t>(y * width_ + x);
    }
};

// Map metadata (for map list)
struct map_info {
    std::string name;
    std::string file_path;
    int32_t width;
    int32_t height;
    bool is_dungeon;
    bool is_pvp;
    int32_t level_requirement;
};

} // namespace hb
