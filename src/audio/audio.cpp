#include "audio/audio.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

bool audio::initialize() {
    spdlog::info("Audio system initialized");
    return true;
}

void audio::shutdown() {
    stop_all_sounds();
    stop_music();
    buffers_.clear();
    active_sounds_.clear();
    spdlog::info("Audio system shutdown");
}

void audio::update(float delta_time) {
    // Remove finished sounds
    active_sounds_.erase(
        std::remove_if(active_sounds_.begin(), active_sounds_.end(),
            [](const active_sound& s) {
                return s.sound->getStatus() == sf::Sound::Status::Stopped;
            }),
        active_sounds_.end()
    );

    // Process music fade
    if (fade_state_ == fade_state::fading_out) {
        fade_timer_ -= delta_time;
        if (fade_timer_ <= 0.0f) {
            // Fade-out complete: stop old track, start new one with fade-in
            music_.stop();
            if (!pending_music_path_.empty()) {
                if (music_.openFromFile(pending_music_path_)) {
                    music_.setLooping(pending_music_loop_);
                    music_.setVolume(0.0f);
                    music_.play();
                    fade_state_ = fade_state::fading_in;
                    fade_timer_ = 0.0f;
                    spdlog::info("Crossfade: starting {}", pending_music_path_);
                } else {
                    spdlog::error("Crossfade: failed to load {}", pending_music_path_);
                    fade_state_ = fade_state::none;
                }
                pending_music_path_.clear();
            } else {
                fade_state_ = fade_state::none;
            }
        } else {
            // Linearly decrease volume during fade-out
            float t = fade_timer_ / fade_duration_;
            float target = effective_volume(music_volume_) * 100.0f;
            music_.setVolume(target * t);
        }
    } else if (fade_state_ == fade_state::fading_in) {
        fade_timer_ += delta_time;
        if (fade_timer_ >= fade_duration_) {
            // Fade-in complete
            music_.setVolume(effective_volume(music_volume_) * 100.0f);
            fade_state_ = fade_state::none;
        } else {
            float t = fade_timer_ / fade_duration_;
            float target = effective_volume(music_volume_) * 100.0f;
            music_.setVolume(target * t);
        }
    }
}

sound_id audio::load_sound(std::string_view path) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(std::string(path))) {
        spdlog::error("Failed to load sound: {}", path);
        return invalid_sound_id;
    }

    sound_id id = next_id_++;
    buffers_[id] = std::move(buffer);
    spdlog::debug("Loaded sound {}: {}", id, path);
    return id;
}

void audio::unload_sound(sound_id id) {
    if (id == invalid_sound_id) return;

    // Stop any playing instances first
    stop_sound(id);
    buffers_.erase(id);
}

void audio::play_sound(sound_id id, float volume, float pan) {
    if (id == invalid_sound_id || muted_) return;

    auto it = buffers_.find(id);
    if (it == buffers_.end()) {
        spdlog::warn("Attempted to play unknown sound id: {}", id);
        return;
    }

    float intended = volume * sound_volume_;
    auto sound = std::make_unique<sf::Sound>(it->second);
    sound->setVolume(effective_volume(intended) * 100.0f);

    // Pan: -1.0 (left) to 1.0 (right) -> position in 3D space
    sound->setPosition({pan * 10.0f, 0.0f, 0.0f});
    sound->setRelativeToListener(true);

    sound->play();
    active_sounds_.push_back({std::move(sound), intended});
}

void audio::stop_sound(sound_id id) {
    if (id == invalid_sound_id) return;

    auto it = buffers_.find(id);
    if (it == buffers_.end()) return;

    // Stop all instances of this sound
    for (auto& s : active_sounds_) {
        if (&s.sound->getBuffer() == &it->second) {
            s.sound->stop();
        }
    }
}

void audio::stop_all_sounds() {
    for (auto& s : active_sounds_) {
        s.sound->stop();
    }
    active_sounds_.clear();
}

bool audio::play_music(std::string_view path, bool loop) {
    if (!music_.openFromFile(std::string(path))) {
        spdlog::error("Failed to load music: {}", path);
        return false;
    }

    music_.setLooping(loop);
    music_.setVolume(effective_volume(music_volume_) * 100.0f);
    music_.play();

    spdlog::info("Playing music: {} (loop={})", path, loop);
    return true;
}

void audio::crossfade_music(std::string_view path, bool loop, float fade_duration) {
    fade_duration_ = std::max(0.1f, fade_duration);

    if (is_music_playing()) {
        // Fade out current track, then fade in new track
        pending_music_path_ = std::string(path);
        pending_music_loop_ = loop;
        fade_state_ = fade_state::fading_out;
        fade_timer_ = fade_duration_;
    } else {
        // No music playing: start new track with fade-in
        if (!music_.openFromFile(std::string(path))) {
            spdlog::error("Failed to load music: {}", path);
            return;
        }
        music_.setLooping(loop);
        music_.setVolume(0.0f);
        music_.play();
        fade_state_ = fade_state::fading_in;
        fade_timer_ = 0.0f;
        spdlog::info("Playing music (fade-in): {} (loop={})", path, loop);
    }
}

void audio::stop_music() {
    // Cancel any active fade
    fade_state_ = fade_state::none;
    pending_music_path_.clear();
    music_.stop();
}

void audio::pause_music() {
    music_.pause();
}

void audio::resume_music() {
    if (music_.getStatus() == sf::Music::Status::Paused) {
        music_.play();
    }
}

bool audio::is_music_playing() const {
    return music_.getStatus() == sf::Music::Status::Playing;
}

void audio::set_music_volume(float volume) {
    music_volume_ = std::clamp(volume, 0.0f, 1.0f);
    // Only apply immediately if no fade is active (fade handles its own volume)
    if (fade_state_ == fade_state::none) {
        music_.setVolume(effective_volume(music_volume_) * 100.0f);
    }
}

void audio::set_master_volume(float volume) {
    master_volume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update music volume
    music_.setVolume(effective_volume(music_volume_) * 100.0f);

    // Update all active sounds using their stored intended volume
    for (auto& s : active_sounds_) {
        s.sound->setVolume(effective_volume(s.intended_volume) * 100.0f);
    }
}

void audio::set_sound_volume(float volume) {
    sound_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void audio::set_muted(bool muted) {
    muted_ = muted;

    if (muted) {
        music_.setVolume(0.0f);
        for (auto& s : active_sounds_) {
            s.sound->setVolume(0.0f);
        }
    } else {
        music_.setVolume(effective_volume(music_volume_) * 100.0f);
        for (auto& s : active_sounds_) {
            s.sound->setVolume(effective_volume(s.intended_volume) * 100.0f);
        }
    }
}

float audio::effective_volume(float base_volume) const {
    if (muted_) return 0.0f;
    return base_volume * master_volume_;
}

} // namespace hb
