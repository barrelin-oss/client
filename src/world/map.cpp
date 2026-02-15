#include "world/map.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <cstring>
#include <cmath>

namespace hb
{

const tile map::null_tile_ = {};

bool map::load(std::string_view path)
{
    std::ifstream file(std::string(path), std::ios::binary);
    if (!file)
    {
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

    if (!file)
    {
        spdlog::error("Failed to read map file: {}", path);
        return false;
    }

    return load_from_memory(data.data(), data.size());
}

bool map::load_from_memory(const uint8_t* data, size_t size)
{
    // AMD file format:
    // Header: 256 bytes text-based config (MAPSIZEX = N, MAPSIZEY = N)
    // Tiles: 10 bytes per tile (terrain_id, terrain_frame, object_id, object_frame, flags, reserved)

    constexpr size_t header_size = 256;
    constexpr size_t tile_data_size = 10;

    if (size < header_size)
    {
        spdlog::error("Map file too small for header (got {} bytes)", size);
        return false;
    }

    // Parse text-based header
    // Replace NULL bytes with spaces for tokenization
    std::string header(reinterpret_cast<const char*>(data), header_size);
    for (char& c : header)
    {
        if (c == '\0')
            c = ' ';
    }

    // Extract MAPSIZEX and MAPSIZEY values
    width_ = 0;
    height_ = 0;

    auto parse_value = [&header](const std::string& key) -> int32_t
    {
        size_t pos = header.find(key);
        if (pos == std::string::npos)
            return 0;

        pos += key.length();
        // Skip whitespace and '='
        while (pos < header.length() && (header[pos] == ' ' || header[pos] == '=' || header[pos] == '\t'))
        {
            ++pos;
        }

        // Parse integer
        int32_t value = 0;
        while (pos < header.length() && header[pos] >= '0' && header[pos] <= '9')
        {
            value = value * 10 + (header[pos] - '0');
            ++pos;
        }
        return value;
    };

    width_ = parse_value("MAPSIZEX");
    height_ = parse_value("MAPSIZEY");

    if (width_ <= 0 || width_ > max_map_width || height_ <= 0 || height_ > max_map_height)
    {
        spdlog::error("Invalid map dimensions: {}x{} (parsed from header)", width_, height_);
        return false;
    }

    size_t expected_size = header_size + static_cast<size_t>(width_ * height_) * tile_data_size;
    if (size < expected_size)
    {
        spdlog::error("Map file too small for tile data (expected {}, got {})", expected_size, size);
        return false;
    }

    // Read tiles (row-major order: Y outer, X inner)
    tiles_.resize(static_cast<size_t>(width_ * height_));

    const uint8_t* tile_data = data + header_size;
    for (int32_t y = 0; y < height_; ++y)
    {
        for (int32_t x = 0; x < width_; ++x)
        {
            size_t file_idx = static_cast<size_t>(y * width_ + x);
            size_t tile_idx = tile_index(x, y);
            const uint8_t* src = tile_data + file_idx * tile_data_size;

            // Read 10-byte tile record (little-endian)
            auto read_i16 = [](const uint8_t* p) -> int16_t
            {
                return static_cast<int16_t>(p[0] | (p[1] << 8));
            };

            tiles_[tile_idx].terrain_id = read_i16(src + 0);
            tiles_[tile_idx].terrain_frame = read_i16(src + 2);
            tiles_[tile_idx].object_id = read_i16(src + 4);
            tiles_[tile_idx].object_frame = read_i16(src + 6);

            // Parse flags byte (byte 8)
            // AMD format: bit 7 = BLOCKED (inverted), bit 6 = TELEPORT, bit 5 = FARM
            uint8_t amd_flags = src[8];
            tile_flag flags = tile_flag::none;

            // Walkable if NOT blocked (bit 7 clear = walkable)
            if ((amd_flags & 0x80) == 0)
            {
                flags = flags | tile_flag::walkable;
            }

            // Teleport (bit 6)
            if ((amd_flags & 0x40) != 0)
            {
                flags = flags | tile_flag::teleport;
            }

            // Water detection: sprite ID 19 is water
            if (tiles_[tile_idx].terrain_id == 19)
            {
                flags = flags | tile_flag::water;
            }

            tiles_[tile_idx].flags = flags;
            tiles_[tile_idx].light_level = 255;
        }
    }

    spdlog::info("Loaded map '{}' ({}x{}, {} tiles)", name_, width_, height_, tiles_.size());
    return true;
}

void map::clear()
{
    name_.clear();
    width_ = 0;
    height_ = 0;
    tiles_.clear();
}

const tile& map::get_tile(int32_t x, int32_t y) const
{
    if (!is_valid_position(x, y))
    {
        return null_tile_;
    }
    return tiles_[tile_index(x, y)];
}

tile& map::get_tile_mut(int32_t x, int32_t y)
{
    static tile dummy;
    if (!is_valid_position(x, y))
    {
        dummy = {};
        return dummy;
    }
    return tiles_[tile_index(x, y)];
}

bool map::is_valid_position(int32_t x, int32_t y) const
{
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

void map::set_tile_flag(int32_t x, int32_t y, tile_flag flag, bool set)
{
    if (!is_valid_position(x, y))
    {
        return;
    }

    auto& t = tiles_[tile_index(x, y)];
    if (set)
    {
        t.flags = t.flags | flag;
    }
    else
    {
        t.flags = static_cast<tile_flag>(static_cast<uint16_t>(t.flags) & ~static_cast<uint16_t>(flag));
    }
}

bool map::is_walkable(int32_t x, int32_t y) const
{
    if (!is_valid_position(x, y))
    {
        return false;
    }
    const auto& t = tiles_[tile_index(x, y)];
    return t.is_walkable();
}

bool map::has_line_of_sight(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const
{
    // Bresenham's line algorithm to check for blocking tiles
    int32_t dx = std::abs(x2 - x1);
    int32_t dy = std::abs(y2 - y1);
    int32_t sx = x1 < x2 ? 1 : -1;
    int32_t sy = y1 < y2 ? 1 : -1;
    int32_t err = dx - dy;

    int32_t x = x1;
    int32_t y = y1;

    while (true)
    {
        if (x == x2 && y == y2)
        {
            return true;
        }

        const auto& t = get_tile(x, y);
        if (t.blocks_sight() && !(x == x1 && y == y1))
        {
            return false;
        }

        int32_t e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }
}

std::optional<std::pair<int32_t, int32_t>> map::find_nearest_walkable(int32_t x, int32_t y, int32_t range) const
{
    if (is_walkable(x, y))
    {
        return std::make_pair(x, y);
    }

    // Spiral search
    for (int32_t r = 1; r <= range; ++r)
    {
        for (int32_t dx = -r; dx <= r; ++dx)
        {
            for (int32_t dy = -r; dy <= r; ++dy)
            {
                if (std::abs(dx) == r || std::abs(dy) == r)
                {
                    int32_t nx = x + dx;
                    int32_t ny = y + dy;
                    if (is_walkable(nx, ny))
                    {
                        return std::make_pair(nx, ny);
                    }
                }
            }
        }
    }

    return std::nullopt;
}

} // namespace hb
