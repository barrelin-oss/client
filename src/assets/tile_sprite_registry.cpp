#include "assets/tile_sprite_registry.hpp"
#include "assets/pak_file.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <cctype>

namespace hb {

bool tile_sprite_registry::initialize(sprite_manager& sprites, std::string_view sprites_path)
{
    sprites_ = &sprites;
    sprites_path_ = std::string(sprites_path);

    // Ensure path ends with separator
    if (!sprites_path_.empty() && sprites_path_.back() != '/' && sprites_path_.back() != '\\')
    {
        sprites_path_ += '/';
    }

    // Register all the hardcoded core PAK mappings first
    register_core_paks();

    // Auto-discover Tile###-###.pak files
    // For filesystem operations, we need the full path relative to working directory
    // sprite_manager is initialized with "assets/" so we prepend that for discovery
    std::string full_fs_path = "assets/" + sprites_path_;
    discover_tile_paks(full_fs_path);

    spdlog::info("Tile sprite registry initialized: {} mappings", id_map_.size());
    return true;
}

const sprite* tile_sprite_registry::get_sprite(int16_t id)
{
    // Check cache first
    auto cache_it = cache_.find(id);
    if (cache_it != cache_.end())
    {
        return cache_it->second.get();
    }

    // Look up the mapping
    auto map_it = id_map_.find(id);
    if (map_it == id_map_.end())
    {
        // No mapping for this ID - cache nullptr to avoid repeated lookups
        cache_[id] = nullptr;
        return nullptr;
    }

    const auto& source = map_it->second;

    // Try to get the PAK file from sprite manager
    pak_file* pak = sprites_->get_pak(source.pak_name);
    if (!pak)
    {
        // PAK not loaded yet - try to load it on demand
        // Build the full path: sprites_path_ + pak_name + .pak
        std::string pak_path = sprites_path_ + source.pak_name + ".pak";
        if (!sprites_->load_pak(source.pak_name, pak_path))
        {
            spdlog::debug("Failed to load PAK file for sprite ID {}: {}", id, pak_path);
            cache_[id] = nullptr;
            return nullptr;
        }
        pak = sprites_->get_pak(source.pak_name);
        if (!pak)
        {
            cache_[id] = nullptr;
            return nullptr;
        }
    }

    // Load the sprite from the PAK
    auto owned_sprite = std::make_unique<sprite>();
    if (owned_sprite->load_from_pak(*pak, source.pak_index))
    {
        // Log first successful load for debugging
        static bool logged_first = false;
        if (!logged_first)
        {
            spdlog::info("First tile sprite loaded: ID {} from {}.pak[{}] - {} frames",
                         id, source.pak_name, source.pak_index, owned_sprite->frame_count());
            logged_first = true;
        }

        const sprite* ptr = owned_sprite.get();
        cache_[id] = std::move(owned_sprite);
        return ptr;
    }

    // Failed to load - cache nullptr
    spdlog::debug("Failed to load sprite ID {} from {}.pak index {}",
                  id, source.pak_name, source.pak_index);
    cache_[id] = nullptr;
    return nullptr;
}

bool tile_sprite_registry::has_mapping(int16_t id) const
{
    return id_map_.find(id) != id_map_.end();
}

void tile_sprite_registry::clear_cache()
{
    cache_.clear();
}

void tile_sprite_registry::register_range(int16_t start_id, std::string_view pak_name, uint32_t count)
{
    std::string pak_str(pak_name);
    for (uint32_t i = 0; i < count; ++i)
    {
        int16_t id = static_cast<int16_t>(start_id + i);
        id_map_[id] = sprite_source{pak_str, i};
    }
    spdlog::debug("Registered {} sprites from {}.pak starting at ID {}", count, pak_name, start_id);
}

void tile_sprite_registry::register_core_paks()
{
    // These mappings are from the original Game.cpp sprite loading code
    // Each PAK file's sprites are loaded into a specific ID range

    // Base terrain tiles - maptiles1.pak (32 sprites at IDs 0-31)
    register_range(0, "maptiles1", 32);

    // Cave interiors - Sinside1.pak (27 sprites at IDs 70-96)
    register_range(70, "Sinside1", 27);

    // Trees - Trees1.pak (46 sprites at IDs 100-145)
    register_range(100, "Trees1", 46);

    // Tree shadows - TreeShadows.pak (46 sprites at IDs 150-195)
    register_range(150, "TreeShadows", 46);

    // Objects - multiple PAK files
    register_range(200, "Objects1", 8);   // IDs 200-207
    register_range(211, "Objects2", 5);   // IDs 211-215
    register_range(216, "Objects3", 4);   // IDs 216-219
    register_range(220, "objects4", 1);   // ID 220

    // Additional terrain - maptiles2.pak (15 sprites at IDs 300-314)
    register_range(300, "maptiles2", 15);

    // maptiles4.pak (10 sprites at IDs 320-329)
    register_range(320, "maptiles4", 10);

    // maptiles5.pak (19 sprites at IDs 330-348)
    register_range(330, "maptiles5", 19);

    // maptiles6.pak (4 sprites at IDs 349-352)
    register_range(349, "maptiles6", 4);

    // maptiles353-361.pak (9 sprites at IDs 353-361)
    register_range(353, "maptiles353-361", 9);

    // Note: Tile###-###.pak files are discovered automatically by discover_tile_paks()
    // This includes Tile223-225, Tile226-229, Tile363-366, etc.
}

void tile_sprite_registry::discover_tile_paks(std::string_view sprites_path)
{
    namespace fs = std::filesystem;

    std::error_code ec;
    fs::path dir(sprites_path);

    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
    {
        spdlog::warn("Sprites directory not found: {}", sprites_path);
        return;
    }

    int discovered_count = 0;

    for (const auto& entry : fs::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::string filename = entry.path().filename().string();

        // Check if it matches Tile###-###.pak pattern
        auto range = parse_tile_pak_name(filename);
        if (!range)
        {
            continue;
        }

        auto [start_id, end_id] = *range;
        uint32_t count = static_cast<uint32_t>(end_id - start_id + 1);

        // Get the pak name without .pak extension
        std::string pak_name = entry.path().stem().string();

        // Check if we already have mappings for this range (from core paks)
        bool already_registered = has_mapping(start_id);
        if (already_registered)
        {
            // Skip - core paks take precedence
            continue;
        }

        // Load the PAK file to verify sprite count
        // (We need to know the actual count in the file)
        // Use relative path for sprite_manager (it prepends asset_root_)
        std::string relative_path = sprites_path_ + pak_name + ".pak";
        if (!sprites_->load_pak(pak_name, relative_path))
        {
            spdlog::warn("Failed to load discovered tile PAK: {}", relative_path);
            continue;
        }

        pak_file* pak = sprites_->get_pak(pak_name);
        if (!pak)
        {
            continue;
        }

        // Use actual sprite count from PAK, but cap at the range count
        uint32_t actual_count = std::min(count, pak->sprite_count());

        register_range(start_id, pak_name, actual_count);
        discovered_count++;
    }

    spdlog::info("Auto-discovered {} Tile###-###.pak files", discovered_count);
}

std::optional<std::pair<int16_t, int16_t>> tile_sprite_registry::parse_tile_pak_name(std::string_view filename)
{
    // Convert to lowercase for case-insensitive matching
    std::string lower_name(filename);
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Match pattern: tile###-###.pak
    std::regex tile_pattern(R"(tile(\d+)-(\d+)\.pak)", std::regex::icase);
    std::smatch match;

    std::string name_str(filename);
    if (!std::regex_match(name_str, match, tile_pattern))
    {
        return std::nullopt;
    }

    try
    {
        int start = std::stoi(match[1].str());
        int end = std::stoi(match[2].str());

        if (start < 0 || start > 32767 || end < 0 || end > 32767 || start > end)
        {
            return std::nullopt;
        }

        return std::make_pair(static_cast<int16_t>(start), static_cast<int16_t>(end));
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

} // namespace hb
