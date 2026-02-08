#include "world/weather_system.hpp"
#include "graphics/renderer.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace hb {

void weather_system::set_weather(weather_type weather)
{
    if (weather_ == weather) return;

    auto old = weather_;
    weather_ = weather;

    // Determine particle count based on intensity
    // Light: 120, Medium: 300, Heavy: 600, Clear: 0
    switch (weather)
    {
        case weather_type::clear:
            active_particle_count_ = 0;
            break;
        case weather_type::light_rain:
        case weather_type::light_snow:
            active_particle_count_ = max_weather_particles / 5;  // 120
            break;
        case weather_type::medium_rain:
        case weather_type::medium_snow:
            active_particle_count_ = max_weather_particles / 2;  // 300
            break;
        case weather_type::heavy_rain:
        case weather_type::heavy_snow:
            active_particle_count_ = max_weather_particles;      // 600
            break;
    }

    particle_accumulator_ = 0.0f;
    spawn_particles();

    spdlog::info("Weather changed: {} -> {} ({} particles)",
        static_cast<int>(old), static_cast<int>(weather), active_particle_count_);
}

void weather_system::set_time(time_of_day time)
{
    if (time_ == time) return;
    time_ = time;
    target_tint_ = tint_for_time(time);
    spdlog::info("Time of day changed to {}", static_cast<int>(time));
}

void weather_system::set_screen_size(uint32_t width, uint32_t height)
{
    screen_width_ = width;
    screen_height_ = height;
}

void weather_system::update(float delta_time)
{
    // Fixed-step particle updates to match legacy 30ms interval.
    // This ensures consistent particle speed regardless of frame rate.
    bool is_rain = is_raining();
    bool is_snow = is_snowing();

    if (is_rain || is_snow)
    {
        particle_accumulator_ += delta_time;

        // Cap accumulator to prevent spiral-of-death (max ~5 steps per frame)
        particle_accumulator_ = std::min(particle_accumulator_, particle_step_interval * 5.0f);

        while (particle_accumulator_ >= particle_step_interval)
        {
            particle_accumulator_ -= particle_step_interval;

            for (int32_t i = 0; i < active_particle_count_; ++i)
            {
                auto& p = particles_[i];

                if (p.step < 0)
                {
                    ++p.step;
                    continue;
                }

                if (is_rain)
                {
                    update_rain_particle(p);
                }
                else
                {
                    update_snow_particle(p);
                }
            }
        }
    }

    // Interpolate tint toward target (smooth, time-based)
    auto lerp_channel = [](uint8_t current, uint8_t target, float t) -> uint8_t
    {
        float result = static_cast<float>(current)
            + (static_cast<float>(target) - static_cast<float>(current)) * t;
        return static_cast<uint8_t>(std::clamp(result, 0.0f, 255.0f));
    };

    // Transition speed: ~2 seconds to fully change
    float t = std::min(delta_time * 0.5f, 1.0f);
    current_tint_.r = lerp_channel(current_tint_.r, target_tint_.r, t);
    current_tint_.g = lerp_channel(current_tint_.g, target_tint_.g, t);
    current_tint_.b = lerp_channel(current_tint_.b, target_tint_.b, t);
    current_tint_.a = lerp_channel(current_tint_.a, target_tint_.a, t);
}

void weather_system::render_particles(renderer& rend)
{
    if (active_particle_count_ <= 0) return;

    bool is_rain = is_raining();
    bool is_snow = is_snowing();
    if (!is_rain && !is_snow) return;

    int32_t sw = static_cast<int32_t>(screen_width_);
    int32_t sh = static_cast<int32_t>(screen_height_);

    for (int32_t i = 0; i < active_particle_count_; ++i)
    {
        const auto& p = particles_[i];
        if (p.step < 0) continue;

        int32_t px = static_cast<int32_t>(p.x);
        int32_t py = static_cast<int32_t>(p.y);

        // Cull off-screen
        if (px < -20 || px > sw + 20 || py < -20 || py > sh + 20) continue;

        if (is_rain)
        {
            if (p.step < 20)
            {
                // Falling raindrop - diagonal line
                uint8_t alpha = static_cast<uint8_t>(std::clamp(180 - p.step * 4, 80, 180));
                sf::Color color(200, 210, 255, alpha);
                rend.draw_line(px, py, px - 2, py + 8, color);
            }
            else
            {
                // Splash - small horizontal line that expands
                int32_t splash_frame = p.step - 20;
                int32_t spread = splash_frame * 2;
                uint8_t alpha = static_cast<uint8_t>(std::max(0, 150 - splash_frame * 30));
                sf::Color color(200, 220, 255, alpha);
                rend.draw_line(px - spread, py, px + spread, py, color);
            }
        }
        else // snow
        {
            // Snowflake - small filled dot
            float life = static_cast<float>(p.step) / 80.0f;
            uint8_t alpha = static_cast<uint8_t>(std::clamp(
                static_cast<int>(200.0f * (1.0f - life * 0.5f)), 80, 200));
            int32_t size = (i % 3 == 0) ? 2 : 1;  // Varying sizes
            sf::Color color(240, 240, 255, alpha);
            rend.draw_rect(px, py, size, size, color, true);
        }
    }
}

void weather_system::render_overlay(renderer& rend)
{
    if (current_tint_.a == 0) return;

    sf::Color tint(current_tint_.r, current_tint_.g, current_tint_.b, current_tint_.a);
    rend.draw_rect(0, 0,
        static_cast<int32_t>(rend.scene_width()),
        static_cast<int32_t>(rend.scene_height()),
        tint, true);
}

bool weather_system::is_raining() const
{
    return weather_ == weather_type::light_rain
        || weather_ == weather_type::medium_rain
        || weather_ == weather_type::heavy_rain;
}

bool weather_system::is_snowing() const
{
    return weather_ == weather_type::light_snow
        || weather_ == weather_type::medium_snow
        || weather_ == weather_type::heavy_snow;
}

void weather_system::spawn_particles()
{
    for (int32_t i = 0; i < active_particle_count_; ++i)
    {
        reset_particle(particles_[i], true);
    }
}

void weather_system::reset_particle(weather_particle& p, bool stagger)
{
    std::uniform_real_distribution<float> dist_x(
        -100.0f, static_cast<float>(screen_width_) + 100.0f);

    p.x = dist_x(rng_);

    if (is_raining())
    {
        // Rain spawns above screen
        std::uniform_real_distribution<float> dist_y(-200.0f, -20.0f);
        p.y = dist_y(rng_);
    }
    else
    {
        // Snow spawns above screen
        std::uniform_real_distribution<float> dist_y(-300.0f, -20.0f);
        p.y = dist_y(rng_);
    }

    if (stagger)
    {
        // Random spawn delay to stagger particles
        std::uniform_int_distribution<int16_t> dist_delay(-40, 0);
        p.step = dist_delay(rng_);

        // Also scatter vertically so they don't all start at the top
        std::uniform_real_distribution<float> dist_scatter(
            -20.0f, static_cast<float>(screen_height_));
        p.y = dist_scatter(rng_);
    }
    else
    {
        p.step = 0;
    }
}

void weather_system::update_rain_particle(weather_particle& p)
{
    // Rain: fast downward + slight leftward drift
    // Legacy: sY += (40 - cStep), sX -= 1, cycle 0-25 at 30ms intervals
    if (p.step < 20)
    {
        float speed = static_cast<float>(40 - p.step);
        p.y += speed * 0.5f;
        p.x -= 0.5f;
        ++p.step;
    }
    else if (p.step <= 25)
    {
        // Splash phase (stays in place)
        ++p.step;
    }
    else
    {
        reset_particle(p, false);
    }
}

void weather_system::update_snow_particle(weather_particle& p)
{
    // Snow: slow downward + random horizontal drift
    // Legacy: sY += (80 - cStep) / 10, sX += 1 - (rand() % 3), cycle 0-80 at 30ms intervals
    if (p.step < 80)
    {
        float speed = static_cast<float>(80 - p.step) / 10.0f;
        p.y += speed * 0.4f;

        // Horizontal waver
        std::uniform_int_distribution<int> drift(-1, 1);
        p.x += static_cast<float>(drift(rng_));

        ++p.step;
    }
    else
    {
        reset_particle(p, false);
    }
}

tint_color weather_system::tint_for_time(time_of_day t)
{
    switch (t)
    {
        case time_of_day::dawn:
            return {200, 140, 80, 35};       // Warm orange
        case time_of_day::morning:
            return {0, 0, 0, 0};             // No tint
        case time_of_day::noon:
            return {0, 0, 0, 0};             // No tint
        case time_of_day::afternoon:
            return {200, 160, 100, 15};      // Warm
        case time_of_day::dusk:
            return {160, 60, 40, 55};        // Orange-red
        case time_of_day::night:
            return {10, 10, 50, 110};        // Dark blue
        case time_of_day::midnight:
            return {5, 5, 30, 140};          // Deep blue
    }
    return {0, 0, 0, 0};
}

} // namespace hb
