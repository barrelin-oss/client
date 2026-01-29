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

void audio::update() {
    // Remove finished sounds
    active_sounds_.erase(
        std::remove_if(active_sounds_.begin(), active_sounds_.end(),
            [](const std::unique_ptr<sf::Sound>& sound) {
                return sound->getStatus() == sf::Sound::Status::Stopped;
            }),
        active_sounds_.end()
    );
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

    auto sound = std::make_unique<sf::Sound>(it->second);
    sound->setVolume(effective_volume(volume) * sound_volume_ * 100.0f);

    // Pan: -1.0 (left) to 1.0 (right) -> position in 3D space
    sound->setPosition({pan * 10.0f, 0.0f, 0.0f});
    sound->setRelativeToListener(true);

    sound->play();
    active_sounds_.push_back(std::move(sound));
}

void audio::stop_sound(sound_id id) {
    if (id == invalid_sound_id) return;

    auto it = buffers_.find(id);
    if (it == buffers_.end()) return;

    // Stop all instances of this sound
    for (auto& sound : active_sounds_) {
        if (&sound->getBuffer() == &it->second) {
            sound->stop();
        }
    }
}

void audio::stop_all_sounds() {
    for (auto& sound : active_sounds_) {
        sound->stop();
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

void audio::stop_music() {
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
    music_.setVolume(effective_volume(music_volume_) * 100.0f);
}

void audio::set_master_volume(float volume) {
    master_volume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update music volume
    music_.setVolume(effective_volume(music_volume_) * 100.0f);

    // Update all active sounds
    for (auto& sound : active_sounds_) {
        float current = sound->getVolume() / 100.0f;
        sound->setVolume(effective_volume(current) * 100.0f);
    }
}

void audio::set_sound_volume(float volume) {
    sound_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void audio::set_muted(bool muted) {
    muted_ = muted;

    if (muted) {
        music_.setVolume(0.0f);
        for (auto& sound : active_sounds_) {
            sound->setVolume(0.0f);
        }
    } else {
        music_.setVolume(effective_volume(music_volume_) * 100.0f);
        // Active sounds will need to be replayed at correct volume
    }
}

float audio::effective_volume(float base_volume) const {
    if (muted_) return 0.0f;
    return base_volume * master_volume_;
}

} // namespace hb
