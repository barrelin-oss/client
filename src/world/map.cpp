#include "world/map.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cstring>
#include <cmath>

namespace hb {

const tile map::null_tile_ = {};

bool map::load(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::binary);
    if (!file) {
        spdlog::error("Failed to open map file: {}", path);
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    // Read entire file
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));

    if (!file) {
        spdlog::error("Failed to read map file: {}", path);
        return false;
    }

    return load_from_memory(data.data(), data.size());
}

bool map::load_from_memory(const uint8_t* data, size_t size) {
    // Helbreath map format (simplified - actual format may vary):
    // Header: map name (32 bytes) + width (4) + height (4)
    // Tiles: terrain_id (2) + object_id (2) + flags (2) per tile

    constexpr size_t header_size = 40;  // name (32) + width (4) + height (4)
    constexpr size_t tile_data_size = 6; // terrain (2) + object (2) + flags (2)

    if (size < header_size) {
        spdlog::error("Map file too small for header");
        return false;
    }

    // Read header
    char name_buf[33] = {0};
    std::memcpy(name_buf, data, 32);
    name_ = name_buf;

    std::memcpy(&width_, data + 32, 4);
    std::memcpy(&height_, data + 36, 4);

    if (width_ <= 0 || width_ > max_map_width ||
        height_ <= 0 || height_ > max_map_height) {
        spdlog::error("Invalid map dimensions: {}x{}", width_, height_);
        return false;
    }

    size_t expected_size = header_size + static_cast<size_t>(width_ * height_) * tile_data_size;
    if (size < expected_size) {
        spdlog::error("Map file too small for tile data (expected {}, got {})", expected_size, size);
        return false;
    }

    // Read tiles
    tiles_.resize(static_cast<size_t>(width_ * height_));

    const uint8_t* tile_data = data + header_size;
    for (size_t i = 0; i < tiles_.size(); ++i) {
        const uint8_t* src = tile_data + i * tile_data_size;

        tiles_[i].terrain_id = static_cast<uint16_t>(src[0]) | (static_cast<uint16_t>(src[1]) << 8);
        tiles_[i].object_id = static_cast<uint16_t>(src[2]) | (static_cast<uint16_t>(src[3]) << 8);
        tiles_[i].flags = static_cast<tile_flag>(src[4] | (src[5] << 8));
        tiles_[i].light_level = 255;
    }

    spdlog::info("Loaded map '{}' ({}x{})", name_, width_, height_);
    return true;
}

void map::clear() {
    name_.clear();
    width_ = 0;
    height_ = 0;
    tiles_.clear();
}

const tile& map::get_tile(int32_t x, int32_t y) const {
    if (!is_valid_position(x, y)) {
        return null_tile_;
    }
    return tiles_[tile_index(x, y)];
}

tile& map::get_tile_mut(int32_t x, int32_t y) {
    static tile dummy;
    if (!is_valid_position(x, y)) {
        dummy = {};
        return dummy;
    }
    return tiles_[tile_index(x, y)];
}

bool map::is_valid_position(int32_t x, int32_t y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

void map::set_tile_occupied(int32_t x, int32_t y, bool occupied) {
    set_tile_flag(x, y, tile_flag::occupied, occupied);
}

void map::set_tile_flag(int32_t x, int32_t y, tile_flag flag, bool set) {
    if (!is_valid_position(x, y)) {
        return;
    }

    auto& t = tiles_[tile_index(x, y)];
    if (set) {
        t.flags = t.flags | flag;
    } else {
        t.flags = static_cast<tile_flag>(static_cast<uint16_t>(t.flags) & ~static_cast<uint16_t>(flag));
    }
}

bool map::is_walkable(int32_t x, int32_t y) const {
    if (!is_valid_position(x, y)) {
        return false;
    }
    const auto& t = tiles_[tile_index(x, y)];
    return t.is_walkable() && !t.is_occupied();
}

bool map::has_line_of_sight(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const {
    // Bresenham's line algorithm to check for blocking tiles
    int32_t dx = std::abs(x2 - x1);
    int32_t dy = std::abs(y2 - y1);
    int32_t sx = x1 < x2 ? 1 : -1;
    int32_t sy = y1 < y2 ? 1 : -1;
    int32_t err = dx - dy;

    int32_t x = x1;
    int32_t y = y1;

    while (true) {
        if (x == x2 && y == y2) {
            return true;
        }

        const auto& t = get_tile(x, y);
        if (t.blocks_sight() && !(x == x1 && y == y1)) {
            return false;
        }

        int32_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

std::optional<std::pair<int32_t, int32_t>> map::find_nearest_walkable(int32_t x, int32_t y, int32_t range) const {
    if (is_walkable(x, y)) {
        return std::make_pair(x, y);
    }

    // Spiral search
    for (int32_t r = 1; r <= range; ++r) {
        for (int32_t dx = -r; dx <= r; ++dx) {
            for (int32_t dy = -r; dy <= r; ++dy) {
                if (std::abs(dx) == r || std::abs(dy) == r) {
                    int32_t nx = x + dx;
                    int32_t ny = y + dy;
                    if (is_walkable(nx, ny)) {
                        return std::make_pair(nx, ny);
                    }
                }
            }
        }
    }

    return std::nullopt;
}

} // namespace hb
