#include "audio/sound_manager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace hb {

namespace fs = std::filesystem;

namespace {

// Case-insensitive string comparison
bool iequals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
        {
            return false;
        }
    }
    return true;
}

// Check if extension is .wav (case-insensitive)
bool is_wav_extension(const fs::path& path)
{
    auto ext = path.extension().string();
    return iequals(ext, ".wav");
}

} // anonymous namespace

bool sound_manager::initialize(audio& audio_system)
{
    audio_ = &audio_system;
    sound_dir_ = "assets/data/sounds/";
    music_dir_ = "assets/data/music/";

    // Initialize arrays to invalid
    character_sounds_.fill(invalid_sound_id);
    effect_sounds_.fill(invalid_sound_id);

    // Load all sounds
    load_sounds();

    spdlog::info("Sound manager initialized");
    return true;
}

void sound_manager::shutdown()
{
    stop_bgm();

    // Unload all sounds
    for (auto id : character_sounds_)
    {
        if (id != invalid_sound_id)
        {
            audio_->unload_sound(id);
        }
    }
    for (auto id : effect_sounds_)
    {
        if (id != invalid_sound_id)
        {
            audio_->unload_sound(id);
        }
    }
    for (auto& [num, id] : monster_sounds_)
    {
        if (id != invalid_sound_id)
        {
            audio_->unload_sound(id);
        }
    }

    character_sounds_.fill(invalid_sound_id);
    effect_sounds_.fill(invalid_sound_id);
    monster_sounds_.clear();

    audio_ = nullptr;
    spdlog::info("Sound manager shutdown");
}

void sound_manager::load_sounds()
{
    if (!fs::exists(sound_dir_))
    {
        spdlog::warn("Sound directory not found: {}", sound_dir_);
        return;
    }

    int loaded_count = 0;

    // Scan directory for sound files
    for (const auto& entry : fs::directory_iterator(sound_dir_))
    {
        if (!entry.is_regular_file()) continue;
        if (!is_wav_extension(entry.path())) continue;

        auto stem = entry.path().stem().string();
        if (stem.empty()) continue;

        char prefix = static_cast<char>(std::toupper(static_cast<unsigned char>(stem[0])));
        if (prefix != 'C' && prefix != 'E' && prefix != 'M') continue;

        // Parse number from filename (e.g., "C14" -> 14)
        int num = 0;
        try
        {
            num = std::stoi(stem.substr(1));
        }
        catch (...)
        {
            continue;  // Not a valid sound filename
        }

        if (num <= 0) continue;

        // Load the sound
        sound_id id = audio_->load_sound(entry.path().string());
        if (id == invalid_sound_id)
        {
            spdlog::warn("Failed to load sound: {}", entry.path().string());
            continue;
        }

        // Store in appropriate array/map
        switch (prefix)
        {
            case 'C':
                if (num < static_cast<int>(character_sounds_.size()))
                {
                    character_sounds_[num] = id;
                    ++loaded_count;
                }
                break;

            case 'E':
                if (num < static_cast<int>(effect_sounds_.size()))
                {
                    effect_sounds_[num] = id;
                    ++loaded_count;
                }
                break;

            case 'M':
                monster_sounds_[num] = id;
                ++loaded_count;
                break;
        }
    }

    spdlog::info("Loaded {} sound effects", loaded_count);
}

sound_id sound_manager::find_and_load_sound(char prefix, int num)
{
    // This is a fallback for on-demand loading (not currently used)
    std::string pattern = std::string(1, prefix) + std::to_string(num);

    if (!fs::exists(sound_dir_)) return invalid_sound_id;

    for (const auto& entry : fs::directory_iterator(sound_dir_))
    {
        if (!entry.is_regular_file()) continue;
        if (!is_wav_extension(entry.path())) continue;

        auto stem = entry.path().stem().string();
        if (iequals(stem, pattern))
        {
            return audio_->load_sound(entry.path().string());
        }
    }

    return invalid_sound_id;
}

sound_id sound_manager::get_sound_id(char type, int num) const
{
    if (num <= 0) return invalid_sound_id;

    switch (std::toupper(static_cast<unsigned char>(type)))
    {
        case 'C':
            if (num < static_cast<int>(character_sounds_.size()))
            {
                return character_sounds_[num];
            }
            break;

        case 'E':
            if (num < static_cast<int>(effect_sounds_.size()))
            {
                return effect_sounds_[num];
            }
            break;

        case 'M':
        {
            auto it = monster_sounds_.find(num);
            if (it != monster_sounds_.end())
            {
                return it->second;
            }
            break;
        }
    }

    return invalid_sound_id;
}

void sound_manager::play_sound(char type, int num, int tile_distance, float pan)
{
    if (!sfx_enabled_ || !audio_) return;
    if (tile_distance > max_sound_distance) return;

    sound_id id = get_sound_id(type, num);
    if (id == invalid_sound_id) return;

    // Calculate volume: 0 tiles = 1.0, max_sound_distance tiles = 0.0
    float volume = 1.0f;
    if (tile_distance > 0)
    {
        volume = 1.0f - (static_cast<float>(tile_distance) / static_cast<float>(max_sound_distance));
    }

    // Clamp pan to valid range
    pan = std::clamp(pan, -1.0f, 1.0f);

    audio_->play_sound(id, volume, pan);
}

void sound_manager::play_ui_sound(int effect_num)
{
    // UI sounds play at full volume with no panning
    play_sound('E', effect_num, 0, 0.0f);
}

void sound_manager::play_sound_at(char type, int num, int32_t world_x, int32_t world_y)
{
    if (!sfx_enabled_ || !audio_) return;

    // Calculate tile positions
    int32_t sound_tile_x = world_x / tile_size;
    int32_t sound_tile_y = world_y / tile_size;
    int32_t listener_tile_x = listener_x_ / tile_size;
    int32_t listener_tile_y = listener_y_ / tile_size;

    // Calculate distance and pan
    int32_t dx = sound_tile_x - listener_tile_x;
    int32_t dy = sound_tile_y - listener_tile_y;

    // Use Chebyshev distance (max of |dx|, |dy|)
    int tile_distance = std::max(std::abs(dx), std::abs(dy));

    // Pan based on horizontal offset
    float pan = std::clamp(static_cast<float>(dx) / static_cast<float>(max_sound_distance), -1.0f, 1.0f);

    play_sound(type, num, tile_distance, pan);
}

void sound_manager::play_character_sound(character_sound sound, int tile_distance, float pan)
{
    play_sound('C', static_cast<int>(sound), tile_distance, pan);
}

void sound_manager::play_character_sound_at(character_sound sound, int32_t world_x, int32_t world_y)
{
    play_sound_at('C', static_cast<int>(sound), world_x, world_y);
}

void sound_manager::set_listener_position(int32_t world_x, int32_t world_y)
{
    listener_x_ = world_x;
    listener_y_ = world_y;
}

std::string sound_manager::select_bgm_track(std::string_view map_name, int weather_type) const
{
    // Convert map name to lowercase for comparison
    std::string map_lower(map_name);
    std::transform(map_lower.begin(), map_lower.end(), map_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Christmas weather (4-6)
    if (weather_type >= 4 && weather_type <= 6)
    {
        return "carol.ogg";
    }

    // Map-specific tracks
    if (map_lower.find("aresden") != std::string::npos)
    {
        return "aresden.ogg";
    }
    if (map_lower.find("elvine") != std::string::npos)
    {
        return "elvine.ogg";
    }
    if (map_lower.starts_with("dglv") || map_lower.starts_with("middled1"))
    {
        return "dungeon.ogg";
    }
    if (map_lower.find("middleland") != std::string::npos)
    {
        return "middleland.ogg";
    }
    if (map_lower.find("abaddon") != std::string::npos)
    {
        return "abaddon.ogg";
    }

    // Default track
    return "maintm.ogg";
}

void sound_manager::start_bgm(std::string_view map_name, int weather_type)
{
    if (!music_enabled_ || !audio_) return;

    std::string track = select_bgm_track(map_name, weather_type);

    // Don't restart if same track already playing
    if (track == current_bgm_track_ && audio_->is_music_playing())
    {
        return;
    }

    // Build full path and crossfade to new track
    std::string path = music_dir_ + track;
    audio_->crossfade_music(path, true);
    current_bgm_track_ = track;
    spdlog::info("Started BGM: {} (map: {})", track, map_name);
}

void sound_manager::play_bgm_track(std::string_view track, bool loop)
{
    if (!music_enabled_ || !audio_) return;

    std::string track_str(track);

    // Don't restart if same track already playing
    if (track_str == current_bgm_track_ && audio_->is_music_playing())
    {
        return;
    }

    std::string path = music_dir_ + track_str;
    audio_->crossfade_music(path, loop);
    current_bgm_track_ = track_str;
    spdlog::info("Started BGM track: {}", track);
}

void sound_manager::stop_bgm()
{
    if (audio_)
    {
        audio_->stop_music();
    }
    current_bgm_track_.clear();
}

bool sound_manager::is_bgm_playing() const
{
    return audio_ && audio_->is_music_playing();
}

void sound_manager::set_sfx_enabled(bool enabled)
{
    sfx_enabled_ = enabled;

    if (!enabled && audio_)
    {
        audio_->stop_all_sounds();
    }
}

void sound_manager::set_music_enabled(bool enabled)
{
    bool was_enabled = music_enabled_;
    music_enabled_ = enabled;

    if (!enabled && audio_)
    {
        audio_->stop_music();
        current_bgm_track_.clear();
    }
    else if (enabled && !was_enabled && audio_)
    {
        // Music was just enabled - caller should call start_bgm with current map
    }
}

} // namespace hb
