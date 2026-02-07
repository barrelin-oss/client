#pragma once

#include "assets/sprite.hpp"
#include "assets/pak_file.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>

namespace hb {

// Sprite cache key
struct sprite_key {
    std::string pak_name;
    uint32_t sprite_index;

    bool operator==(const sprite_key& other) const {
        return pak_name == other.pak_name && sprite_index == other.sprite_index;
    }
};

} // namespace hb

// Hash specialization for sprite_key
template<>
struct std::hash<hb::sprite_key> {
    size_t operator()(const hb::sprite_key& key) const {
        size_t h1 = std::hash<std::string>{}(key.pak_name);
        size_t h2 = std::hash<uint32_t>{}(key.sprite_index);
        return h1 ^ (h2 << 1);
    }
};

namespace hb {

class sprite_manager {
public:
    sprite_manager() = default;
    ~sprite_manager() = default;

    sprite_manager(const sprite_manager&) = delete;
    sprite_manager& operator=(const sprite_manager&) = delete;

    // Initialize with asset root path
    bool initialize(std::string_view asset_root);
    void shutdown();

    // PAK file management
    bool load_pak(std::string_view name, std::string_view path);
    pak_file* get_pak(std::string_view name);
    void unload_pak(std::string_view name);

    // Sprite loading
    // Returns mutable pointer since sprites need to be modified during rendering
    // (e.g., for lazy bitmap loading, usage tracking)
    sprite* get_sprite(std::string_view pak_name, uint32_t index);
    sprite* get_sprite_by_type(uint16_t sprite_type);

    // Preload sprites for faster access
    void preload_sprites(std::string_view pak_name, uint32_t start, uint32_t count);

    // Cache management
    void clear_cache();
    void shrink_cache(size_t max_entries);

    // Statistics
    size_t cached_sprite_count() const { return sprite_cache_.size(); }
    size_t loaded_pak_count() const { return pak_files_.size(); }

    // === Memory management (legacy-style) ===
    // Call periodically to evict unused bitmaps and free GPU memory
    void update_memory(float delta_time);
    // Set eviction timeout (sprites unused for this long get their bitmap unloaded)
    void set_eviction_timeout(float seconds) { eviction_timeout_ = seconds; }
    // Get current GPU memory usage (approximate)
    size_t gpu_memory_usage() const;
    // Get count of sprites with loaded bitmaps
    size_t loaded_bitmap_count() const;

    // === Metadata preloading (load headers at startup, bitmaps on-demand) ===
    // Preload all sprite metadata from a PAK (fast, low memory)
    void preload_pak_metadata(std::string_view pak_name);
    // Preload metadata for specific sprite range
    void preload_metadata_range(std::string_view pak_name, uint32_t start, uint32_t count);

    // ID-based sprite storage (original game style: m_pSprite[ID])
    // Store a sprite at a specific ID slot
    bool store_sprite_at_id(uint16_t id, std::string_view pak_name, uint32_t index);
    // Get sprite by ID (returns nullptr if not loaded)
    sprite* get_sprite_by_id(uint16_t id);
    const sprite* get_sprite_by_id(uint16_t id) const;
    // Check if ID slot has a sprite
    bool has_sprite_at_id(uint16_t id) const;

    // Common PAK files
    static constexpr const char* pak_tiles = "tiles";
    static constexpr const char* pak_effects = "effects";
    static constexpr const char* pak_interface = "interface";
    static constexpr const char* pak_objects = "objects";
    static constexpr const char* pak_malehu = "malehu";     // Male human
    static constexpr const char* pak_femalehu = "femalehu"; // Female human
    static constexpr const char* pak_monsters = "monsters";

private:
    std::string asset_root_;
    std::unordered_map<std::string, std::unique_ptr<pak_file>> pak_files_;
    std::unordered_map<sprite_key, std::unique_ptr<sprite>> sprite_cache_;
    std::unordered_map<uint16_t, sprite*> id_sprites_;  // ID-based sprite references

    // LRU tracking (simplified - just track last access order)
    std::vector<sprite_key> lru_order_;

    // Memory management
    float eviction_timeout_ = 30.0f;        // Seconds before unused bitmaps are evicted
    float eviction_check_timer_ = 0.0f;     // Timer for periodic eviction checks
    static constexpr float eviction_check_interval_ = 5.0f;  // Check every 5 seconds
};

// Sprite type to PAK mapping (from original game)
struct sprite_type_info {
    const char* pak_name;
    uint32_t base_index;
    uint32_t count;
};

// Common sprite type ranges
namespace sprite_types {
    // Tiles and terrain (tiles.pak)
    inline constexpr uint16_t tile_grass = 100;
    inline constexpr uint16_t tile_dirt = 200;
    inline constexpr uint16_t tile_stone = 300;
    inline constexpr uint16_t tile_water = 400;

    // Objects (objects.pak)
    inline constexpr uint16_t object_tree = 1000;
    inline constexpr uint16_t object_rock = 1100;
    inline constexpr uint16_t object_building = 1200;

    // Effects (effects.pak)
    inline constexpr uint16_t effect_fire = 2000;
    inline constexpr uint16_t effect_magic = 2100;
    inline constexpr uint16_t effect_hit = 2200;

    // Characters (malehu.pak / femalehu.pak)
    inline constexpr uint16_t char_body = 3000;
    inline constexpr uint16_t char_armor = 3100;
    inline constexpr uint16_t char_weapon = 3200;
    inline constexpr uint16_t char_shield = 3300;
    inline constexpr uint16_t char_helm = 3400;
    inline constexpr uint16_t char_hair = 3500;

    // UI (interface.pak)
    inline constexpr uint16_t ui_dialog = 4000;
    inline constexpr uint16_t ui_button = 4100;
    inline constexpr uint16_t ui_icon = 4200;
}

} // namespace hb
