#include "ui/status_log.hpp"
#include "graphics/renderer.hpp"
#include <algorithm>
#include <cmath>

namespace hb {

void status_log::update(float delta_time)
{
    // Update animation timers
    flash_timer_ += delta_time;
    if (flash_timer_ > 1.0f)
    {
        flash_timer_ -= 1.0f;
    }

    rainbow_time_ += delta_time;
    // Keep rainbow_time_ from growing unbounded
    if (rainbow_time_ > 100.0f)
    {
        rainbow_time_ -= 100.0f;
    }

    // Update keyed message timers and remove expired ones
    for (auto& msg : messages_)
    {
        msg.elapsed += delta_time;
    }

    messages_.erase(
        std::remove_if(messages_.begin(), messages_.end(),
            [](const status_message& msg) {
                return msg.lifetime > 0.0f && msg.elapsed >= msg.lifetime;
            }),
        messages_.end()
    );

    // Update event timers and remove expired ones
    for (auto& evt : events_)
    {
        evt.elapsed += delta_time;
    }

    events_.erase(
        std::remove_if(events_.begin(), events_.end(),
            [](const event_message& e) { return e.elapsed >= e.lifetime; }),
        events_.end()
    );
}

void status_log::render(renderer& rend, int32_t /*screen_width*/, int32_t screen_height,
                        int32_t icon_panel_height)
{
    if (messages_.empty() && events_.empty())
    {
        return;
    }

    // Position: bottom-left, above the icon panel
    constexpr int32_t padding_left = 8;
    constexpr int32_t padding_bottom = 4;
    constexpr int32_t line_height = 20;
    constexpr uint32_t font_size = 14;
    const sf::Color outline_color = sf::Color::Black;

    // Start y position (bottom of all messages)
    int32_t y = screen_height - icon_panel_height - padding_bottom;
    int32_t x = padding_left;

    // Helper to calculate fade alpha
    auto calc_fade_alpha = [](float elapsed, float lifetime) -> uint8_t {
        if (lifetime <= 0.0f)
        {
            return 255;  // Persistent messages don't fade
        }
        float remaining = lifetime - elapsed;
        if (remaining < fade_duration && remaining > 0.0f)
        {
            return static_cast<uint8_t>(255 * (remaining / fade_duration));
        }
        return 255;
    };

    // Render events first (at the bottom, newest at bottom)
    // Iterate in reverse so newest (back of deque) is rendered at the bottom
    for (auto it = events_.rbegin(); it != events_.rend(); ++it)
    {
        const auto& evt = *it;
        y -= line_height;

        uint8_t alpha = calc_fade_alpha(evt.elapsed, evt.lifetime);
        sf::Color outline_with_alpha = outline_color;
        outline_with_alpha.a = alpha;

        // Determine rendering method based on color type
        if (evt.color == message_color::rainbow)
        {
            // Rainbow text with outline (whole text same color, cycling)
            rend.draw_text_rainbow(evt.text, x, y, rainbow_time_, font_size, alpha,
                                   true, outline_color);
        }
        else if (evt.color == message_color::special)
        {
            // Per-letter rainbow gradient with outline
            rend.draw_text_special(evt.text, x, y, rainbow_time_, font_size, alpha,
                                   true, outline_color);
        }
        else if (evt.color == message_color::terror)
        {
            // Terror text with outline (bouncing letters)
            sf::Color terror_color = message_color_to_sfml(message_color::red);
            rend.draw_text_terror(evt.text, x, y, rainbow_time_, terror_color, font_size, alpha,
                                  true, outline_color);
        }
        else
        {
            // Static color with outline
            sf::Color color = message_color_to_sfml(evt.color);
            color.a = alpha;
            rend.draw_text_outlined(evt.text, x, y, color, outline_with_alpha, font_size, 1.0f);
        }
    }

    // Render keyed status messages above events (newest at bottom)
    for (auto it = messages_.rbegin(); it != messages_.rend(); ++it)
    {
        const auto& msg = *it;
        y -= line_height;

        // Determine color based on severity
        sf::Color color;
        switch (msg.severity)
        {
            case status_severity::info:
                color = sf::Color(200, 200, 200);  // Light gray
                break;
            case status_severity::warning:
                color = sf::Color(255, 220, 100);  // Yellow
                break;
            case status_severity::critical:
                // Flash between red and white
                if (flash_timer_ < 0.5f)
                {
                    color = sf::Color(255, 80, 80);  // Red
                }
                else
                {
                    color = sf::Color(255, 200, 200);  // Light red/pink
                }
                break;
        }

        // Fade out if message is expiring
        color.a = calc_fade_alpha(msg.elapsed, msg.lifetime);
        sf::Color outline_with_alpha = outline_color;
        outline_with_alpha.a = color.a;

        // Outlined text for readability
        rend.draw_text_outlined(msg.text, x, y, color, outline_with_alpha, font_size, 1.0f);
    }
}

void status_log::set_message(const std::string& key, const std::string& text,
                             status_severity severity, float lifetime)
{
    // Look for existing message with this key
    for (auto& msg : messages_)
    {
        if (msg.key == key)
        {
            // Update existing message
            msg.text = text;
            msg.severity = severity;
            msg.lifetime = lifetime;
            msg.elapsed = 0.0f;  // Reset timer on update
            return;
        }
    }

    // Add new message
    messages_.push_back({
        .key = key,
        .text = text,
        .severity = severity,
        .lifetime = lifetime,
        .elapsed = 0.0f
    });
}

void status_log::remove_message(const std::string& key)
{
    messages_.erase(
        std::remove_if(messages_.begin(), messages_.end(),
            [&key](const status_message& msg) { return msg.key == key; }),
        messages_.end()
    );
}

bool status_log::has_message(const std::string& key) const
{
    for (const auto& msg : messages_)
    {
        if (msg.key == key)
        {
            return true;
        }
    }
    return false;
}

void status_log::add_event(std::string_view text, message_color color, bool allow_duplicates)
{
    // Check for duplicates if not allowed
    if (!allow_duplicates && !events_.empty())
    {
        if (events_.back().text == text)
        {
            return;
        }
    }

    // Add new event
    events_.push_back({
        .text = std::string(text),
        .color = color,
        .lifetime = event_lifetime,
        .elapsed = 0.0f
    });

    // Enforce max events limit (remove oldest)
    while (events_.size() > max_events)
    {
        events_.pop_front();
    }
}

void status_log::clear()
{
    messages_.clear();
    events_.clear();
}

void status_log::clear_events()
{
    events_.clear();
}

} // namespace hb
