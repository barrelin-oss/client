#pragma once

#include <SFML/Audio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hb {

using sound_id = uint16_t;
inline constexpr sound_id invalid_sound_id = 0;

class audio {
public:
    bool initialize();
    void shutdown();

    // Call each frame to clean up finished sounds and process fades
    void update(float delta_time);

    // Sound effects
    sound_id load_sound(std::string_view path);
    void unload_sound(sound_id id);
    void play_sound(sound_id id, float volume = 1.0f, float pan = 0.0f);
    void stop_sound(sound_id id);
    void stop_all_sounds();

    // Music (streaming)
    bool play_music(std::string_view path, bool loop = true);
    void stop_music();
    void pause_music();
    void resume_music();
    bool is_music_playing() const;
    void set_music_volume(float volume);

    // Crossfade to a new music track
    void crossfade_music(std::string_view path, bool loop = true, float fade_duration = 1.0f);

    // Global volume control (0.0 to 1.0)
    void set_master_volume(float volume);
    float master_volume() const { return master_volume_; }

    void set_sound_volume(float volume);
    float sound_volume() const { return sound_volume_; }

    void set_muted(bool muted);
    bool is_muted() const { return muted_; }

    // Debug stats
    size_t active_sound_count() const { return active_sounds_.size(); }

private:
    float effective_volume(float base_volume) const;

    struct active_sound
    {
        std::unique_ptr<sf::Sound> sound;
        float intended_volume = 1.0f;
    };

    std::unordered_map<sound_id, sf::SoundBuffer> buffers_;
    std::vector<active_sound> active_sounds_;
    sf::Music music_;

    float master_volume_ = 1.0f;
    float sound_volume_ = 1.0f;
    float music_volume_ = 1.0f;
    bool muted_ = false;

    // Music fade state
    enum class fade_state : uint8_t { none, fading_out, fading_in };
    fade_state fade_state_ = fade_state::none;
    float fade_timer_ = 0.0f;
    float fade_duration_ = 1.0f;
    std::string pending_music_path_;
    bool pending_music_loop_ = true;

    sound_id next_id_ = 1;
};

} // namespace hb
