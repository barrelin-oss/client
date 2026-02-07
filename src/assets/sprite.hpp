#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string_view>
#include <vector>
#include <chrono>

namespace hb {

class pak_file;
struct pak_sprite_data;
struct pak_sprite_metadata;

// Individual frame within a sprite
struct sprite_frame {
    sf::IntRect source_rect;  // Region in texture
    int16_t pivot_x;          // X offset for drawing
    int16_t pivot_y;          // Y offset for drawing
};

// Sprite loaded from PAK file
// Supports two-tier loading: metadata first, bitmap on-demand
class sprite {
public:
    sprite() = default;
    ~sprite() = default;

    sprite(const sprite&) = delete;
    sprite& operator=(const sprite&) = delete;
    sprite(sprite&&) noexcept = default;
    sprite& operator=(sprite&&) noexcept = default;

    // === Metadata-only loading (fast, small footprint) ===
    // Load just frame data from PAK without loading bitmap
    bool load_metadata_from_pak(pak_file& pak, uint32_t index);
    bool load_metadata(const pak_sprite_metadata& metadata);

    // === Full loading (metadata + bitmap) ===
    // Load from PAK file (metadata + bitmap together)
    bool load_from_pak(pak_file& pak, uint32_t index);
    // Load from image file directly (for testing)
    bool load_from_file(std::string_view path);
    // Load from raw data
    bool load_from_data(const pak_sprite_data& data);

    // === On-demand bitmap loading ===
    // Load bitmap data (call when sprite needs to render)
    // Requires metadata to be loaded first and pak_file pointer set
    bool load_bitmap();
    // Unload bitmap to free memory (keeps metadata)
    void unload_bitmap();

    // === State queries ===
    // Has metadata (frames, dimensions) loaded?
    bool has_metadata() const { return metadata_loaded_; }
    // Has bitmap (texture) loaded?
    bool is_loaded() const { return bitmap_loaded_; }
    // Ensure bitmap is loaded (loads on-demand if needed)
    bool ensure_loaded() const;

    // === Memory management ===
    // Update last-used timestamp (called on draw)
    void touch() const;
    // Get time since last use
    float seconds_since_use() const;
    // Get approximate memory usage in bytes
    size_t memory_usage() const;

    // === Frame access ===
    uint32_t frame_count() const { return static_cast<uint32_t>(frames_.size()); }
    const sprite_frame& get_frame(uint32_t index) const;

    // === Drawing (with color key transparency - default) ===
    void draw(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame = 0) const;
    void draw_alpha(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame, float alpha) const;

    // === Variable-width drawing (for HP/MP bars) ===
    // Renders only the left portion of the sprite up to 'width' pixels
    // Used for gauge bar fills that need to show partial amounts
    void draw_width(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame, int32_t width) const;

    // === Drawing without color key (for backgrounds, etc.) ===
    void draw_no_color_key(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame = 0) const;
    void draw_alpha_no_color_key(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame, float alpha) const;

    // Get bounds for collision detection
    sf::IntRect get_bounds(int32_t x, int32_t y, uint32_t frame) const;

    // Get texture/bitmap dimensions (from metadata, no bitmap load needed)
    uint32_t bitmap_width() const { return bitmap_width_; }
    uint32_t bitmap_height() const { return bitmap_height_; }

    // Direct texture access (for custom rendering) - loads bitmap if needed
    const sf::Texture& texture() const;

    // Set PAK source for on-demand loading
    void set_pak_source(pak_file* pak, uint32_t index) { pak_source_ = pak; pak_index_ = index; }

private:
    // Textures (only populated when bitmap is loaded)
    sf::Texture texture_;              // Texture WITH color key applied (transparent)
    sf::Texture texture_no_colorkey_;  // Texture WITHOUT color key (original colors)

    // Metadata (loaded first, always kept)
    std::vector<sprite_frame> frames_;
    uint32_t bitmap_width_ = 0;
    uint32_t bitmap_height_ = 0;
    sf::Color color_key_;              // The color key (from pixel 0,0)

    // Source for on-demand loading
    pak_file* pak_source_ = nullptr;
    uint32_t pak_index_ = 0;

    // State flags
    bool metadata_loaded_ = false;
    bool bitmap_loaded_ = false;

    // Memory management - mutable for const draw methods
    mutable std::chrono::steady_clock::time_point last_used_;

    static const sprite_frame null_frame_;
};

} // namespace hb
