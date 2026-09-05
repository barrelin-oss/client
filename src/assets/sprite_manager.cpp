#include "assets/sprite_manager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace hb
{

bool sprite_manager::initialize(std::string_view asset_root)
{
    asset_root_ = asset_root;

    // Ensure asset root exists
    if (!std::filesystem::exists(asset_root))
    {
        spdlog::warn("Asset root does not exist: {}", asset_root);
    }

    spdlog::info("Sprite manager initialized with root: {}", asset_root);
    return true;
}

void sprite_manager::shutdown()
{
    clear_cache();
    pak_files_.clear();
    spdlog::info("Sprite manager shutdown");
}

bool sprite_manager::load_pak(std::string_view name, std::string_view path)
{
    std::string name_str(name);

    // Check if already loaded
    if (pak_files_.find(name_str) != pak_files_.end())
    {
        return true;
    }

    // Build full path
    std::filesystem::path full_path = asset_root_;
    full_path /= path;

    auto pak = std::make_unique<pak_file>();
    if (!pak->open(full_path.string()))
    {
        spdlog::error("Failed to load PAK file: {}", full_path.string());
        return false;
    }

    pak_files_[name_str] = std::move(pak);
    spdlog::info(
        "Loaded PAK file '{}': {} ({} sprites)", name, full_path.string(), pak_files_[name_str]->sprite_count());
    return true;
}

pak_file* sprite_manager::get_pak(std::string_view name)
{
    auto it = pak_files_.find(std::string(name));
    if (it != pak_files_.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void sprite_manager::unload_pak(std::string_view name)
{
    std::string name_str(name);

    // Remove all sprites from this PAK from cache
    for (auto it = sprite_cache_.begin(); it != sprite_cache_.end();)
    {
        if (it->first.pak_name == name_str)
        {
            sprite* dying = it->second.get();
            std::erase_if(id_sprites_, [dying](const auto& pair) { return pair.second == dying; });
            it = sprite_cache_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Remove from LRU
    lru_order_.erase(std::remove_if(lru_order_.begin(),
                                    lru_order_.end(),
                                    [&name_str](const sprite_key& key) { return key.pak_name == name_str; }),
                     lru_order_.end());

    pak_files_.erase(name_str);
}

sprite* sprite_manager::get_sprite(std::string_view pak_name, uint32_t index)
{
    sprite_key key{std::string(pak_name), index};

    // Check cache first
    auto it = sprite_cache_.find(key);
    if (it != sprite_cache_.end())
    {
        // Update LRU
        auto lru_it = std::find(lru_order_.begin(), lru_order_.end(), key);
        if (lru_it != lru_order_.end())
        {
            lru_order_.erase(lru_it);
        }
        lru_order_.push_back(key);

        return it->second.get();
    }

    // Load from PAK
    pak_file* pak = get_pak(pak_name);
    if (!pak)
    {
        spdlog::warn("PAK file not loaded: {}", pak_name);
        return nullptr;
    }

    if (index >= pak->sprite_count())
    {
        spdlog::warn("Sprite index out of range: {}[{}]", pak_name, index);
        return nullptr;
    }

    auto spr = std::make_unique<sprite>();

    if (!spr->load_from_pak(*pak, index))
    {
        spdlog::error("Failed to load sprite: {}[{}]", pak_name, index);
        return nullptr;
    }

    sprite* result = spr.get();
    sprite_cache_[key] = std::move(spr);
    lru_order_.push_back(key);

    return result;
}

sprite* sprite_manager::get_sprite_by_type(uint16_t sprite_type)
{
    // Map sprite type to PAK and index
    // This is a simplified mapping - real game has more complex logic

    const char* pak_name = nullptr;
    uint32_t index = 0;

    if (sprite_type >= sprite_types::ui_dialog && sprite_type < sprite_types::ui_dialog + 1000)
    {
        pak_name = pak_interface;
        index = sprite_type - sprite_types::ui_dialog;
    }
    else if (sprite_type >= sprite_types::char_body && sprite_type < sprite_types::char_body + 1000)
    {
        pak_name = pak_malehu; // Default to male human
        index = sprite_type - sprite_types::char_body;
    }
    else if (sprite_type >= sprite_types::effect_fire && sprite_type < sprite_types::effect_fire + 1000)
    {
        pak_name = pak_effects;
        index = sprite_type - sprite_types::effect_fire;
    }
    else if (sprite_type >= sprite_types::object_tree && sprite_type < sprite_types::object_tree + 1000)
    {
        pak_name = pak_objects;
        index = sprite_type - sprite_types::object_tree;
    }
    else if (sprite_type >= sprite_types::tile_grass && sprite_type < sprite_types::tile_grass + 1000)
    {
        pak_name = pak_tiles;
        index = sprite_type - sprite_types::tile_grass;
    }
    else
    {
        spdlog::warn("Unknown sprite type: {}", sprite_type);
        return nullptr;
    }

    return get_sprite(pak_name, index);
}

void sprite_manager::preload_sprites(std::string_view pak_name, uint32_t start, uint32_t count)
{
    pak_file* pak = get_pak(pak_name);
    if (!pak)
    {
        spdlog::warn("Cannot preload from unloaded PAK: {}", pak_name);
        return;
    }

    uint32_t end = std::min(start + count, pak->sprite_count());
    uint32_t loaded = 0;

    for (uint32_t i = start; i < end; ++i)
    {
        if (get_sprite(pak_name, i) != nullptr)
        {
            ++loaded;
        }
    }

    spdlog::trace("Preloaded {} sprites from {}[{}..{}]", loaded, pak_name, start, end - 1);
}

void sprite_manager::clear_cache()
{
    sprite_cache_.clear();
    lru_order_.clear();
    id_sprites_.clear();
}

void sprite_manager::shrink_cache(size_t max_entries)
{
    while (sprite_cache_.size() > max_entries && !lru_order_.empty())
    {
        const sprite_key& oldest = lru_order_.front();
        auto cache_it = sprite_cache_.find(oldest);
        if (cache_it != sprite_cache_.end())
        {
            sprite* dying = cache_it->second.get();
            std::erase_if(id_sprites_, [dying](const auto& pair) { return pair.second == dying; });
            sprite_cache_.erase(cache_it);
        }
        lru_order_.erase(lru_order_.begin());
    }
}

bool sprite_manager::store_sprite_at_id(uint16_t id, std::string_view pak_name, uint32_t index)
{
    // Packs keep empty records for the directions a figure does not have (Gate, the Olympia
    // officers): no frames, a 2x2 bitmap. Nothing to draw, nothing to report.
    if (auto* pak = get_pak(pak_name))
    {
        auto meta = pak->read_sprite_metadata(index); // nullopt when the record has no frames
        if (!meta || meta->frames.empty())
        {
            spdlog::debug("Sprite {}[{}] has no frames, id {} left empty", pak_name, index, id);
            return false;
        }
    }

    // Load the sprite first
    sprite* spr = get_sprite(pak_name, index);
    if (!spr)
    {
        spdlog::error("Failed to load sprite for ID {}: {}[{}]", id, pak_name, index);
        return false;
    }

    // Store reference in ID map
    id_sprites_[id] = spr;
    return true;
}

sprite* sprite_manager::get_sprite_by_id(uint16_t id)
{
    auto it = id_sprites_.find(id);
    if (it != id_sprites_.end())
    {
        return it->second;
    }
    return nullptr;
}

const sprite* sprite_manager::get_sprite_by_id(uint16_t id) const
{
    auto it = id_sprites_.find(id);
    if (it != id_sprites_.end())
    {
        return it->second;
    }
    return nullptr;
}

bool sprite_manager::has_sprite_at_id(uint16_t id) const
{
    return id_sprites_.find(id) != id_sprites_.end();
}

// === Memory management ===

void sprite_manager::update_memory(float delta_time)
{
    eviction_check_timer_ += delta_time;

    if (eviction_check_timer_ < eviction_check_interval_)
    {
        return;
    }

    eviction_check_timer_ = 0.0f;

    // Check all cached sprites for eviction
    size_t evicted = 0;
    size_t memory_freed = 0;

    for (auto& [key, spr] : sprite_cache_)
    {
        if (spr && spr->is_loaded())
        {
            float idle_time = spr->seconds_since_use();
            if (idle_time > eviction_timeout_)
            {
                memory_freed += spr->memory_usage();
                spr->unload_bitmap();
                evicted++;
            }
        }
    }

    if (evicted > 0)
    {
        spdlog::trace("Memory eviction: unloaded {} sprites, freed ~{} KB", evicted, memory_freed / 1024);
    }
}

size_t sprite_manager::gpu_memory_usage() const
{
    size_t total = 0;
    for (const auto& [key, spr] : sprite_cache_)
    {
        if (spr)
        {
            total += spr->memory_usage();
        }
    }
    return total;
}

size_t sprite_manager::loaded_bitmap_count() const
{
    size_t count = 0;
    for (const auto& [key, spr] : sprite_cache_)
    {
        if (spr && spr->is_loaded())
        {
            count++;
        }
    }
    return count;
}

// === Metadata preloading ===

void sprite_manager::preload_pak_metadata(std::string_view pak_name)
{
    pak_file* pak = get_pak(pak_name);
    if (!pak)
    {
        spdlog::warn("Cannot preload metadata from unloaded PAK: {}", pak_name);
        return;
    }

    preload_metadata_range(pak_name, 0, pak->sprite_count());
}

void sprite_manager::preload_metadata_range(std::string_view pak_name, uint32_t start, uint32_t count)
{
    pak_file* pak = get_pak(pak_name);
    if (!pak)
    {
        spdlog::warn("Cannot preload from unloaded PAK: {}", pak_name);
        return;
    }

    uint32_t end = std::min(start + count, pak->sprite_count());
    uint32_t loaded = 0;

    for (uint32_t i = start; i < end; ++i)
    {
        sprite_key key{std::string(pak_name), i};

        // Skip if already cached
        if (sprite_cache_.find(key) != sprite_cache_.end())
        {
            continue;
        }

        auto spr = std::make_unique<sprite>();

        if (spr->load_metadata_from_pak(*pak, i))
        {
            sprite_cache_[key] = std::move(spr);
            lru_order_.push_back(key);
            loaded++;
        }
    }

    spdlog::trace("Preloaded metadata for {} sprites from {}[{}..{}]", loaded, pak_name, start, end - 1);
}

} // namespace hb
