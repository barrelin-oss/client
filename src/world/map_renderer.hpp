#pragma once

#include "world/map.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace hb {

class pak_file;

// Map rendering configuration
struct map_render_config {
    bool show_terrain = true;
    bool show_objects = true;
    bool show_roofs = true;
    bool show_grid = false;
    bool show_walkability = false;
    float light_level = 1.0f;
};

class map_renderer {
public:
    map_renderer() = default;
    ~map_renderer() = default;

    map_renderer(const map_renderer&) = delete;
    map_renderer& operator=(const map_renderer&) = delete;

    // Initialize with sprite data
    bool initialize(pak_file& terrain_pak, pak_file& object_pak);
    void shutdown();

    // Render map
    void render(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y);
    void render_layer(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y, int layer);

    // Configuration
    void set_config(const map_render_config& config) { config_ = config; }
    const map_render_config& config() const { return config_; }

    // Get tile at screen position
    std::pair<int32_t, int32_t> screen_to_tile(int32_t screen_x, int32_t screen_y,
                                                int32_t camera_x, int32_t camera_y) const;
    std::pair<int32_t, int32_t> tile_to_screen(int32_t tile_x, int32_t tile_y,
                                                int32_t camera_x, int32_t camera_y) const;

private:
    // Load sprite for tile
    const sprite* get_terrain_sprite(uint16_t id);
    const sprite* get_object_sprite(uint16_t id);

    // Calculate visible tile range
    struct visible_range {
        int32_t start_x, start_y;
        int32_t end_x, end_y;
    };
    visible_range calculate_visible_range(const map& m, int32_t camera_x, int32_t camera_y) const;

    pak_file* terrain_pak_ = nullptr;
    pak_file* object_pak_ = nullptr;

    std::unordered_map<uint16_t, std::unique_ptr<sprite>> terrain_sprites_;
    std::unordered_map<uint16_t, std::unique_ptr<sprite>> object_sprites_;

    map_render_config config_;
};

} // namespace hb
