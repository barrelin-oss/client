#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hb {

// Frame information from PAK file (matches stBrush struct)
struct sprite_frame_info {
    int16_t source_x;      // sx - X position in bitmap
    int16_t source_y;      // sy - Y position in bitmap
    int16_t width;         // szx - Frame width
    int16_t height;        // szy - Frame height
    int16_t pivot_x;       // pvx - X offset for drawing
    int16_t pivot_y;       // pvy - Y offset for drawing
};

// Sprite metadata only (no bitmap data) - small footprint
struct pak_sprite_metadata {
    std::vector<sprite_frame_info> frames; // Frame definitions
    uint32_t bitmap_width = 0;
    uint32_t bitmap_height = 0;
    uint32_t bitmap_offset = 0;            // Offset to bitmap data in PAK file
};

// Raw sprite data extracted from PAK (includes bitmap)
struct pak_sprite_data {
    std::vector<uint8_t> bitmap_data;      // Raw BMP data (converted to RGBA)
    std::vector<sprite_frame_info> frames; // Frame definitions
    uint32_t bitmap_width = 0;
    uint32_t bitmap_height = 0;
};

class pak_file {
public:
    pak_file() = default;
    ~pak_file();

    pak_file(const pak_file&) = delete;
    pak_file& operator=(const pak_file&) = delete;
    pak_file(pak_file&&) noexcept;
    pak_file& operator=(pak_file&&) noexcept;

    bool open(std::string_view path);
    void close();
    bool is_open() const { return file_.is_open(); }

    // Get the path this pak file was opened from
    std::string_view path() const { return path_; }

    // Get number of sprites in the PAK file
    uint32_t sprite_count() const { return sprite_count_; }

    // Read sprite data at given index (full load - metadata + bitmap)
    std::optional<pak_sprite_data> read_sprite(uint32_t index);

    // Read sprite metadata only (fast, no bitmap loading)
    std::optional<pak_sprite_metadata> read_sprite_metadata(uint32_t index);

    // Read sprite bitmap only (for on-demand loading after metadata)
    // Returns RGBA bitmap data
    std::optional<std::vector<uint8_t>> read_sprite_bitmap(uint32_t index);

private:
    bool read_header();
    std::optional<pak_sprite_data> read_sprite_internal(uint32_t offset);
    std::optional<pak_sprite_metadata> read_metadata_internal(uint32_t offset);
    std::optional<std::vector<uint8_t>> read_bitmap_internal(uint32_t offset);

    std::ifstream file_;
    std::string path_;
    std::vector<uint32_t> sprite_offsets_;
    uint32_t sprite_count_ = 0;
};

} // namespace hb
