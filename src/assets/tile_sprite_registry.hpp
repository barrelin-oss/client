#pragma once

#include "assets/sprite.hpp"
#include "assets/sprite_manager.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <vector>

namespace hb {

// Maps legacy sprite IDs to (pak_name, pak_index) pairs
// This mirrors the original game's hardcoded sprite loading system where
// multiple PAK files are loaded into specific array index ranges.
//
// Legacy ID ranges (from Game.cpp):
//   0-31     maptiles1      Base terrain
//   70-96    Sinside1       Cave interiors
//   100-145  Trees1         Trees
//   150-195  TreeShadows    Tree shadows
//   200-207  Objects1       Objects
//   211-215  Objects2       Objects
//   216-219  Objects3       Objects
//   220      Objects4       Objects
//   223-225  Tile223-225    Tiles
//   ...and many more Tile###-###.pak files auto-discovered
class tile_sprite_registry
{
public:
    tile_sprite_registry() = default;
    ~tile_sprite_registry() = default;

    tile_sprite_registry(const tile_sprite_registry&) = delete;
    tile_sprite_registry& operator=(const tile_sprite_registry&) = delete;

    // Initialize the registry with all tile PAK mappings
    // sprites_path: path to the sprites directory (e.g., "assets/sprites/")
    bool initialize(sprite_manager& sprites, std::string_view sprites_path);

    // Two-phase initialization for loading screen integration:
    // Phase 1: register core mappings + discover tile PAKs (returns list, doesn't load)
    struct pending_tile_pak {
        std::string pak_name;
        std::string relative_path;
        int16_t start_id;
        uint32_t count;
    };
    bool initialize_core(sprite_manager& sprites, std::string_view sprites_path);
    std::vector<pending_tile_pak> discover_tile_pak_list();
    void load_tile_pak(const pending_tile_pak& entry);

    // Get a sprite by its legacy ID
    // Returns nullptr if the ID is not mapped or the sprite fails to load
    const sprite* get_sprite(int16_t id);

    // Check if an ID has a mapping (doesn't load the sprite)
    bool has_mapping(int16_t id) const;

    // Get statistics
    size_t registered_count() const { return id_map_.size(); }
    size_t loaded_count() const { return cache_.size(); }

    // Clear the sprite cache (keeps mappings)
    void clear_cache();

private:
    // Source information for a sprite ID
    struct sprite_source
    {
        std::string pak_name;  // PAK file name (e.g., "maptiles1")
        uint32_t pak_index;    // Index within the PAK file
    };

    // Register a range of sprite IDs from a PAK file
    // start_id: the first legacy ID in the range
    // pak_name: name of the PAK file (without .pak extension)
    // count: number of sprites to register
    void register_range(int16_t start_id, std::string_view pak_name, uint32_t count);

    // Register all the hardcoded core PAK mappings
    void register_core_paks();

    // Auto-discover and register Tile###-###.pak files
    void discover_tile_paks(std::string_view sprites_path);

    // Parse a Tile###-###.pak filename to extract start and end IDs
    // Returns (start_id, end_id) or nullopt if parsing fails
    std::optional<std::pair<int16_t, int16_t>> parse_tile_pak_name(std::string_view filename);

    // ID -> source mapping
    std::unordered_map<int16_t, sprite_source> id_map_;

    // Sprite cache (lazy-loaded)
    std::unordered_map<int16_t, std::unique_ptr<sprite>> cache_;

    // Sprite manager reference for loading
    sprite_manager* sprites_ = nullptr;

    // Path to sprites directory (for on-demand PAK loading)
    std::string sprites_path_;
};

} // namespace hb
