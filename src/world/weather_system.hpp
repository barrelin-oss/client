#pragma once

#include "world/world.hpp"
#include <array>
#include <cstdint>
#include <random>

namespace hb
{

class renderer;

// Weather particle for rain/snow effects
struct weather_particle
{
    float x = 0.0f;   // Screen-space X
    float y = 0.0f;   // Screen-space Y
    int16_t step = 0; // Animation step (-N = spawn delay, 0+ = active)
};

inline constexpr int32_t max_weather_particles = 600;

class weather_system
{
public:
    weather_system() = default;
    ~weather_system() = default;

    weather_system(const weather_system&) = delete;
    weather_system& operator=(const weather_system&) = delete;

    // Set current weather (triggers particle spawn/despawn)
    void set_weather(weather_type weather);
    weather_type weather() const { return weather_; }

    // Set screen dimensions (particles span the viewport)
    void set_screen_size(uint32_t width, uint32_t height);

    // Update particles and transitions
    void update(float delta_time);

    // Render weather particles (call in scene space, after game world)
    void render_particles(renderer& rend);

    // Is rain currently active (for audio)
    bool is_raining() const;

    // Is snow currently active (for audio/BGM)
    bool is_snowing() const;

    // Visibility toggle (hides particles without stopping updates)
    void set_visible(bool v) { visible_ = v; }
    bool visible() const { return visible_; }

private:
    void spawn_particles();
    void reset_particle(weather_particle& p, bool stagger);

    // Particle animation (called at fixed 30ms intervals)
    void update_rain_particle(weather_particle& p);
    void update_snow_particle(weather_particle& p);

    bool visible_ = true;
    weather_type weather_ = weather_type::clear;

    uint32_t screen_width_ = 640;
    uint32_t screen_height_ = 480;

    // Particle pool
    std::array<weather_particle, max_weather_particles> particles_{};
    int32_t active_particle_count_ = 0;

    // Time accumulator for fixed-step particle updates (legacy: 30ms per step)
    float particle_accumulator_ = 0.0f;
    static constexpr float particle_step_interval = 0.030f; // 30ms

    // RNG
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace hb
