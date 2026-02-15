#pragma once

#include "world/map.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite.hpp"
#include <SFML/Graphics/RenderTexture.hpp>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hb
{

class tile_sprite_registry;

// Map rendering configuration
struct map_render_config
{
    bool show_terrain = true;
    bool show_objects = true;
    bool show_roofs = true;
    bool show_grid = false;
    bool show_walkability = false;
    float light_level = 1.0f;
};

class map_renderer
{
public:
    map_renderer() = default;
    ~map_renderer() = default;

    map_renderer(const map_renderer&) = delete;
    map_renderer& operator=(const map_renderer&) = delete;

    // Visible tile range returned by get_visible_range()
    struct visible_range
    {
        int32_t start_x, start_y;
        int32_t end_x, end_y;
    };

    // Initialize with tile sprite registry
    bool initialize(tile_sprite_registry& registry);
    void shutdown();

    // Render full map (terrain + objects + debug overlays)
    void render(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y);

    // Render terrain chunks and debug overlays only (no objects)
    void render_terrain(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y);

    // Render objects for a single tile row
    // If player_bounds is provided, objects overlapping it are drawn semi-transparently
    void render_objects_row(renderer& rend,
                            const map& m,
                            int32_t row_y,
                            int32_t camera_x,
                            int32_t camera_y,
                            const sf::IntRect* player_bounds = nullptr);

    // Get visible tile range for current camera position
    visible_range get_visible_range(const map& m, int32_t camera_x, int32_t camera_y) const;

    // Configuration
    void set_config(const map_render_config& config) { config_ = config; }
    const map_render_config& config() const { return config_; }

    // Update screen dimensions for visible range calculation
    void set_screen_size(uint32_t width, uint32_t height);

    // Update zoom level for visible range calculation
    void set_zoom_level(float level) { zoom_level_ = level; }

    // Per-frame object render count (reset each frame by caller)
    int32_t objects_rendered() const { return objects_rendered_; }
    void reset_objects_rendered() { objects_rendered_ = 0; }

    // Debug stats
    size_t chunk_count() const { return chunks_.size(); }

    // Pathfinding debug trace (rendered as cyan tiles when show_walkability is on)
    void set_pathfinding_trace(std::vector<std::pair<int32_t, int32_t>> trace)
    {
        pathfinding_trace_ = std::move(trace);
    }
    void clear_pathfinding_trace() { pathfinding_trace_.clear(); }

    // Occupied tile debug overlay (yellow tint when show_walkability is on)
    void set_occupied_tiles(std::vector<std::pair<int32_t, int32_t>> tiles) { occupied_tiles_ = std::move(tiles); }
    void clear_occupied_tiles() { occupied_tiles_.clear(); }

    // Invalidate all chunk caches (call when map changes)
    void invalidate_chunks();

    // Get tile at screen position
    std::pair<int32_t, int32_t>
    screen_to_tile(int32_t screen_x, int32_t screen_y, int32_t camera_x, int32_t camera_y) const;
    std::pair<int32_t, int32_t>
    tile_to_screen(int32_t tile_x, int32_t tile_y, int32_t camera_x, int32_t camera_y) const;

private:
    // Get sprite for tile (terrain and objects use the same registry)
    const sprite* get_tile_sprite(int16_t id);

    // Internal visible range calculation
    visible_range calculate_visible_range(const map& m, int32_t camera_x, int32_t camera_y) const;

    // Chunk-based rendering
    static constexpr int32_t chunk_size_ = 16; // 16x16 tiles per chunk

    struct chunk_key
    {
        int32_t cx, cy;
        bool operator==(const chunk_key& other) const { return cx == other.cx && cy == other.cy; }
    };
    struct chunk_key_hash
    {
        size_t operator()(const chunk_key& k) const
        {
            auto combined = (static_cast<int64_t>(static_cast<uint32_t>(k.cx)) << 32)
                          | static_cast<uint32_t>(k.cy);
            return std::hash<int64_t>()(combined);
        }
    };

    struct chunk_data
    {
        std::unique_ptr<sf::RenderTexture> texture;
        bool valid = false;
    };

    std::unordered_map<chunk_key, chunk_data, chunk_key_hash> chunks_;

    void render_chunk(const map& m, int32_t chunk_x, int32_t chunk_y);
    chunk_data& get_or_create_chunk(int32_t chunk_x, int32_t chunk_y);

    // Render objects tile-by-tile (always on top of chunk terrain)
    void render_objects(renderer& rend, const map& m, int32_t camera_x, int32_t camera_y);

    tile_sprite_registry* registry_ = nullptr;
    map_render_config config_;
    std::vector<std::pair<int32_t, int32_t>> pathfinding_trace_;
    std::vector<std::pair<int32_t, int32_t>> occupied_tiles_;
    int32_t objects_rendered_ = 0;

    // Screen dimensions for calculating visible tile range
    uint32_t screen_width_ = 640;
    uint32_t screen_height_ = 480;
    float zoom_level_ = 1.0f; // 1.0 = normal, >1 = zoomed out (more tiles visible)
};

} // namespace hb
