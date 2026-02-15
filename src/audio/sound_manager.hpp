#pragma once

#include "audio/audio.hpp"
#include "audio/sound_types.hpp"
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

namespace hb
{

// Maximum audible distance in tiles (0 = full volume, >14 = silent)
inline constexpr int max_sound_distance = 14;

// Tile size in pixels
inline constexpr int tile_size = 32;

class sound_manager
{
public:
    sound_manager() = default;
    ~sound_manager() = default;

    sound_manager(const sound_manager&) = delete;
    sound_manager& operator=(const sound_manager&) = delete;

    // Initialize sound manager and load all sounds
    bool initialize(audio& audio_system);
    void shutdown();

    // Spatial sound - attenuated by distance, panned by position
    // type: 'C' for character, 'E' for effect, 'M' for monster
    // num: sound number (e.g., 14 for E14.wav)
    // tile_distance: 0-14 tiles (0 = full volume, >14 = silent)
    // pan: -1.0 (left) to +1.0 (right)
    void play_sound(char type, int num, int tile_distance, float pan = 0.0f);

    // UI sound - plays at full volume with no panning
    // Convenience wrapper for non-spatial sounds (button clicks, etc.)
    void play_ui_sound(int effect_num);

    // Ambient looping sound (e.g., rain) — only one ambient at a time
    void start_ambient(char type, int num);
    void stop_ambient();

    // Spatial sound at world position - calculates distance/pan from listener
    void play_sound_at(char type, int num, int32_t world_x, int32_t world_y);

    // Character sound helpers using type-safe enum
    // See sound_types.hpp for character_sound enum and helper functions
    void play_character_sound(character_sound sound, int tile_distance = 0, float pan = 0.0f);
    void play_character_sound_at(character_sound sound, int32_t world_x, int32_t world_y);

    // Set listener position for play_sound_at calculations (in pixels)
    void set_listener_position(int32_t world_x, int32_t world_y);

    // Background music
    void start_bgm(std::string_view map_name, int weather_type = 0);
    void play_bgm_track(std::string_view track, bool loop = true);
    void stop_bgm();
    bool is_bgm_playing() const;

    // Enable/disable
    void set_sfx_enabled(bool enabled);
    void set_music_enabled(bool enabled);
    bool is_sfx_enabled() const { return sfx_enabled_; }
    bool is_music_enabled() const { return music_enabled_; }

    // Debug stats
    std::string_view current_bgm_track() const { return current_bgm_track_; }

private:
    // Load sounds from directory
    void load_sounds();
    sound_id find_and_load_sound(char prefix, int num);

    // Get sound ID for type and number
    sound_id get_sound_id(char type, int num) const;

    // Select music track based on map name and weather
    std::string select_bgm_track(std::string_view map_name, int weather_type) const;

    audio* audio_ = nullptr;

    // Sound buffers - indexed by sound number
    // Character sounds: C1-C24
    std::array<sound_id, 25> character_sounds_{}; // Index 0 unused, 1-24 valid

    // Effect sounds: E1-E53 (with gaps)
    std::array<sound_id, 60> effect_sounds_{}; // Index 0 unused, room for expansion

    // Monster sounds: M1-M156 (sparse, many gaps)
    std::unordered_map<int, sound_id> monster_sounds_;

    // Current BGM track
    std::string current_bgm_track_;

    // Listener position for spatial audio (in world pixels)
    int32_t listener_x_ = 0;
    int32_t listener_y_ = 0;

    // Enable/disable flags
    bool sfx_enabled_ = true;
    bool music_enabled_ = true;

    // Currently looping ambient sound (0 = none)
    sound_id ambient_sound_id_ = invalid_sound_id;

    // Sound directory path
    std::string sound_dir_;
    std::string music_dir_;
};

} // namespace hb
