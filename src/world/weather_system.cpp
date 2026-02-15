#include "world/weather_system.hpp"
#include "graphics/renderer.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace hb
{

void weather_system::set_weather(weather_type weather)
{
    if (weather_ == weather)
        return;

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
        active_particle_count_ = max_weather_particles / 5; // 120
        break;
    case weather_type::medium_rain:
    case weather_type::medium_snow:
        active_particle_count_ = max_weather_particles / 2; // 300
        break;
    case weather_type::heavy_rain:
    case weather_type::heavy_snow:
        active_particle_count_ = max_weather_particles; // 600
        break;
    }

    particle_accumulator_ = 0.0f;
    spawn_particles();

    spdlog::info("Weather changed: {} -> {} ({} particles)",
                 static_cast<int>(old),
                 static_cast<int>(weather),
                 active_particle_count_);
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
}

void weather_system::render_particles(renderer& rend)
{
    if (!visible_)
        return;
    if (active_particle_count_ <= 0)
        return;

    bool is_rain = is_raining();
    bool is_snow = is_snowing();
    if (!is_rain && !is_snow)
        return;

    int32_t sw = static_cast<int32_t>(screen_width_);
    int32_t sh = static_cast<int32_t>(screen_height_);

    for (int32_t i = 0; i < active_particle_count_; ++i)
    {
        const auto& p = particles_[i];
        if (p.step < 0)
            continue;

        int32_t px = static_cast<int32_t>(p.x);
        int32_t py = static_cast<int32_t>(p.y);

        // Cull off-screen
        if (px < -20 || px > sw + 20 || py < -20 || py > sh + 20)
            continue;

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
            uint8_t alpha = static_cast<uint8_t>(std::clamp(static_cast<int>(200.0f * (1.0f - life * 0.5f)), 80, 200));
            int32_t size = (i % 3 == 0) ? 2 : 1; // Varying sizes
            sf::Color color(240, 240, 255, alpha);
            rend.draw_rect(px, py, size, size, color, true);
        }
    }
}

bool weather_system::is_raining() const
{
    return weather_ == weather_type::light_rain || weather_ == weather_type::medium_rain ||
           weather_ == weather_type::heavy_rain;
}

bool weather_system::is_snowing() const
{
    return weather_ == weather_type::light_snow || weather_ == weather_type::medium_snow ||
           weather_ == weather_type::heavy_snow;
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
    float sw = static_cast<float>(screen_width_);
    float sh = static_cast<float>(screen_height_);

    std::uniform_real_distribution<float> dist_x(-100.0f, sw + 100.0f);
    p.x = dist_x(rng_);

    if (stagger)
    {
        // Initial spawn: scatter across full screen with random step progress
        std::uniform_real_distribution<float> dist_y(-20.0f, sh);
        p.y = dist_y(rng_);

        // Random step so particles are mid-animation, not all starting fresh
        if (is_raining())
        {
            std::uniform_int_distribution<int16_t> dist_step(-10, 15);
            p.step = dist_step(rng_);
        }
        else
        {
            std::uniform_int_distribution<int16_t> dist_step(-10, 60);
            p.step = dist_step(rng_);
        }
    }
    else
    {
        // Respawn: pick a random target Y anywhere on screen, then
        // back-calculate a start position above it so the particle
        // falls naturally to that spot.
        std::uniform_real_distribution<float> dist_target(0.0f, sh);
        float target_y = dist_target(rng_);

        if (is_raining())
        {
            // Pick a random number of steps the particle will have "already fallen"
            // so it enters from just above the screen toward the target Y.
            // Rain step i moves (40-i) px. Start from step 0 above screen.
            // We want p.y to be above screen (negative) so it falls into view.
            // Just place it at target_y minus some travel distance.
            std::uniform_int_distribution<int16_t> dist_offset(100, 300);
            p.y = target_y - static_cast<float>(dist_offset(rng_));
        }
        else
        {
            // Snow is slower — spread the start position more
            std::uniform_int_distribution<int16_t> dist_offset(50, 200);
            p.y = target_y - static_cast<float>(dist_offset(rng_));
        }
        p.step = 0;
    }
}

void weather_system::update_rain_particle(weather_particle& p)
{
    // Rain: fast downward + slight leftward drift
    // Legacy: sY += (40 - cStep), sX -= 1, cycle 0-25 at 30ms intervals
    // Total Y travel at legacy values: sum(40..21) = 610px — covers full screen from spawn
    if (p.step < 20)
    {
        p.y += static_cast<float>(40 - p.step);
        p.x -= 1.0f;
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
    // Total Y travel: sum((80-i)/10 for i=0..79) = ~320px — slower, more floaty
    if (p.step < 80)
    {
        p.y += static_cast<float>(80 - p.step) / 10.0f;

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

} // namespace hb
