#include "gameplay/floating_text.hpp"
#include "graphics/renderer.hpp"
#include "graphics/text_renderer.hpp"
#include <algorithm>
#include <cmath>

namespace hb {

void floating_text_manager::add(floating_text_entry entry)
{
    // Enforce max entries (remove oldest if full)
    if (entries_.size() >= max_entries)
    {
        entries_.erase(entries_.begin());
    }
    entries_.push_back(std::move(entry));
}

void floating_text_manager::add_damage(int32_t amount, float world_x, float world_y)
{
    floating_text_entry entry;
    entry.text = "-" + std::to_string(amount);
    entry.style.color = sf::Color(255, 100, 100);
    entry.style.outline_color = sf::Color::Black;
    entry.style.outline_thickness = 1.0f;
    entry.style.size = 16;
    entry.style.effect = text_effect::none;
    entry.world_x = world_x;
    entry.world_y = world_y;
    entry.lifetime = 1.5f;
    entry.velocity_y = -60.0f;
    add(std::move(entry));
}

void floating_text_manager::add_heal(int32_t amount, float world_x, float world_y)
{
    floating_text_entry entry;
    entry.text = "+" + std::to_string(amount);
    entry.style.color = sf::Color(100, 255, 100);
    entry.style.outline_color = sf::Color::Black;
    entry.style.outline_thickness = 1.0f;
    entry.style.size = 16;
    entry.style.effect = text_effect::none;
    entry.world_x = world_x;
    entry.world_y = world_y;
    entry.lifetime = 1.5f;
    entry.velocity_y = -50.0f;
    add(std::move(entry));
}

void floating_text_manager::add_critical(int32_t amount, float world_x, float world_y)
{
    floating_text_entry entry;
    entry.text = "-" + std::to_string(amount) + "!";
    entry.style.color = sf::Color(255, 50, 50);
    entry.style.outline_color = sf::Color(80, 0, 0);
    entry.style.outline_thickness = 1.5f;
    entry.style.size = 22;
    entry.style.effect = text_effect::glow;
    entry.world_x = world_x;
    entry.world_y = world_y;
    entry.lifetime = 2.0f;
    entry.velocity_y = -80.0f;
    add(std::move(entry));
}

void floating_text_manager::add_text(std::string_view text, float world_x, float world_y, sf::Color color)
{
    floating_text_entry entry;
    entry.text = std::string(text);
    entry.style.color = color;
    entry.style.outline_color = sf::Color::Black;
    entry.style.outline_thickness = 1.0f;
    entry.style.size = 14;
    entry.style.effect = text_effect::none;
    entry.world_x = world_x;
    entry.world_y = world_y;
    entry.lifetime = 1.2f;
    entry.velocity_y = -45.0f;
    add(std::move(entry));
}

void floating_text_manager::update(float delta_time)
{
    for (auto& entry : entries_)
    {
        entry.elapsed += delta_time;
        entry.world_x += entry.velocity_x * delta_time;
        entry.world_y += entry.velocity_y * delta_time;
    }

    // Remove expired entries
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [](const floating_text_entry& e) { return e.elapsed >= e.lifetime; }),
        entries_.end()
    );
}

void floating_text_manager::render(renderer& rend, int32_t camera_x, int32_t camera_y)
{
    // Extended mode: floating text outside fair zone is hidden
    bool extended_cull = rend.current_view_mode() == view_mode::extended;
    sf::IntRect fair;
    if (extended_cull) fair = rend.fair_bounds();

    for (const auto& entry : entries_)
    {
        // World to screen conversion
        auto screen_x = static_cast<int32_t>(entry.world_x) - camera_x;
        auto screen_y = static_cast<int32_t>(entry.world_y) - camera_y;

        // Skip if off screen (with generous margin)
        if (screen_x < -200 || screen_x > static_cast<int32_t>(rend.scene_width()) + 200 ||
            screen_y < -100 || screen_y > static_cast<int32_t>(rend.scene_height()) + 100)
        {
            continue;
        }

        // Extended mode: additionally check fair zone bounds
        if (extended_cull) {
            if (screen_x < fair.position.x - 64 || screen_x > fair.position.x + fair.size.x + 64 ||
                screen_y < fair.position.y - 64 || screen_y > fair.position.y + fair.size.y)
                continue;
        }

        // Fade out during last 25% of lifetime
        float fade_start = entry.lifetime * 0.75f;
        uint8_t alpha = 255;
        if (entry.elapsed > fade_start)
        {
            float fade_progress = (entry.elapsed - fade_start) / (entry.lifetime - fade_start);
            alpha = static_cast<uint8_t>(255.0f * (1.0f - fade_progress));
        }

        // Create a copy of style with faded alpha
        text_style style = entry.style;
        style.color.a = alpha;
        style.outline_color.a = alpha;

        rend.text().draw(entry.text, screen_x, screen_y, style, entry.elapsed);
    }
}

void floating_text_manager::clear()
{
    entries_.clear();
}

} // namespace hb
