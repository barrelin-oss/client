#include "assets/tile_sprite_registry.hpp"
#include "assets/pak_file.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <charconv>

namespace hb
{

bool tile_sprite_registry::initialize(sprite_manager& sprites, std::string_view sprites_path)
{
    if (!initialize_core(sprites, sprites_path))
        return false;

    // Load all discovered tile PAKs synchronously (legacy path)
    auto pending = discover_tile_pak_list();
    for (const auto& entry : pending)
        load_tile_pak(entry);

    spdlog::info("Tile sprite registry initialized: {} mappings", id_map_.size());
    return true;
}

bool tile_sprite_registry::initialize_core(sprite_manager& sprites, std::string_view sprites_path)
{
    sprites_ = &sprites;
    sprites_path_ = std::string(sprites_path);

    if (!sprites_path_.empty() && sprites_path_.back() != '/' && sprites_path_.back() != '\\')
    {
        sprites_path_ += '/';
    }

    register_core_paks();
    return true;
}

std::vector<tile_sprite_registry::pending_tile_pak> tile_sprite_registry::discover_tile_pak_list()
{
    namespace fs = std::filesystem;
    std::vector<pending_tile_pak> result;

    std::string full_fs_path = "assets/" + sprites_path_;
    std::error_code ec;
    fs::path dir(full_fs_path);

    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
    {
        spdlog::warn("Sprites directory not found for tile discovery: {}", full_fs_path);
        return result;
    }

    for (const auto& entry : fs::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file())
            continue;

        std::string filename = entry.path().filename().string();
        auto range = parse_tile_pak_name(filename);
        if (!range)
            continue;

        auto [start_id, end_id] = *range;
        if (has_mapping(start_id))
            continue;

        uint32_t count = static_cast<uint32_t>(end_id - start_id + 1);
        std::string pak_name = entry.path().stem().string();
        std::string relative_path = sprites_path_ + pak_name + ".pak";

        result.push_back({pak_name, relative_path, start_id, count});
    }

    spdlog::info("Discovered {} tile PAK files to load", result.size());
    return result;
}

void tile_sprite_registry::load_tile_pak(const pending_tile_pak& entry)
{
    if (!sprites_->load_pak(entry.pak_name, entry.relative_path))
    {
        spdlog::warn("Failed to load discovered tile PAK: {}", entry.relative_path);
        return;
    }

    pak_file* pak = sprites_->get_pak(entry.pak_name);
    if (!pak)
        return;

    uint32_t actual_count = std::min(entry.count, pak->sprite_count());
    register_range(entry.start_id, entry.pak_name, actual_count);
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
                         id,
                         source.pak_name,
                         source.pak_index,
                         owned_sprite->frame_count());
            logged_first = true;
        }

        const sprite* ptr = owned_sprite.get();
        cache_[id] = std::move(owned_sprite);
        return ptr;
    }

    // Failed to load - cache nullptr
    spdlog::debug("Failed to load sprite ID {} from {}.pak index {}", id, source.pak_name, source.pak_index);
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

    // Buildings - Structures1.pak (20 sprites at IDs 50-69; the 3.82 client loads the pack sprite by sprite)
    register_range(50, "Structures1", 20);

    // Objects - the 3.82 Game.cpp list (objects1 grew to 10 and objects4 to 2 with 3.82)
    register_range(200, "Objects1", 10); // IDs 200-209
    register_range(211, "Objects2", 5);  // IDs 211-215
    register_range(216, "Objects3", 4);  // IDs 216-219
    register_range(220, "objects4", 2);  // IDs 220-221
    register_range(230, "Objects5", 9);  // IDs 230-238
    register_range(238, "Objects6", 4);  // IDs 238-241 (the legacy table overlaps 238 too)
    register_range(242, "Objects7", 7);  // IDs 242-248
    register_range(249, "lgn_objects", 1); // ID 249 (Olympia, next in its load order)

    // Later terrain sets (maptiles2 is registered below at 300; TileNNN-MMM packs are discovered by name)
    register_range(315, "lgn_maptiles", 5);    // IDs 315-319 (Olympia; sits between maptiles2 and maptiles4 in its load order)
    register_range(320, "maptiles4", 10);      // IDs 320-329
    register_range(330, "maptiles5", 19);      // IDs 330-348
    register_range(349, "maptiles6", 4);       // IDs 349-352
    register_range(353, "maptiles353-361", 9); // IDs 353-361

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

// Legacy synchronous discover - kept for backward compatibility via initialize()
void tile_sprite_registry::discover_tile_paks(std::string_view /*sprites_path*/)
{
    auto pending = discover_tile_pak_list();
    for (const auto& entry : pending)
        load_tile_pak(entry);
}

std::optional<std::pair<int16_t, int16_t>> tile_sprite_registry::parse_tile_pak_name(std::string_view filename)
{
    // Match pattern: Tile###-###.pak (case-insensitive)
    // Avoid std::regex - it's extremely slow in libstdc++
    if (filename.size() < 12) // "Tile0-0.pak" minimum
        return std::nullopt;

    // Check "tile" prefix (case-insensitive)
    if (std::tolower(static_cast<unsigned char>(filename[0])) != 't' ||
        std::tolower(static_cast<unsigned char>(filename[1])) != 'i' ||
        std::tolower(static_cast<unsigned char>(filename[2])) != 'l' ||
        std::tolower(static_cast<unsigned char>(filename[3])) != 'e')
        return std::nullopt;

    // Check ".pak" suffix (case-insensitive)
    auto suffix = filename.substr(filename.size() - 4);
    if (std::tolower(static_cast<unsigned char>(suffix[0])) != '.' ||
        std::tolower(static_cast<unsigned char>(suffix[1])) != 'p' ||
        std::tolower(static_cast<unsigned char>(suffix[2])) != 'a' ||
        std::tolower(static_cast<unsigned char>(suffix[3])) != 'k')
        return std::nullopt;

    // Extract the middle: "###-###"
    auto middle = filename.substr(4, filename.size() - 8);
    auto dash = middle.find('-');
    if (dash == std::string_view::npos || dash == 0 || dash == middle.size() - 1)
        return std::nullopt;

    int start = 0, end = 0;
    auto [p1, ec1] = std::from_chars(middle.data(), middle.data() + dash, start);
    auto [p2, ec2] = std::from_chars(middle.data() + dash + 1, middle.data() + middle.size(), end);

    if (ec1 != std::errc{} || ec2 != std::errc{})
        return std::nullopt;
    if (p1 != middle.data() + dash || p2 != middle.data() + middle.size())
        return std::nullopt;
    if (start < 0 || start > 32767 || end < 0 || end > 32767 || start > end)
        return std::nullopt;

    return std::make_pair(static_cast<int16_t>(start), static_cast<int16_t>(end));
}

} // namespace hb
