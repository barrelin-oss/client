#include "ui/screens/connection_lost_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void connection_lost_screen::on_enter()
{
    elapsed_time_ = 0.0f;
    spdlog::info("Connection lost screen entered");
}

void connection_lost_screen::on_exit()
{
    reason_.clear();
    spdlog::info("Connection lost screen exited");
}

bool connection_lost_screen::update(float delta_time, const input& inp)
{
    (void)inp;

    elapsed_time_ += delta_time;

    // Store mouse position for cursor rendering
    mouse_x_ = inp.mouse_x();
    mouse_y_ = inp.mouse_y();

    if (elapsed_time_ >= timeout_duration_)
    {
        if (on_timeout_)
        {
            on_timeout_();
        }
    }

    return true;
}

void connection_lost_screen::render(renderer& rend, sprite_manager& sprites)
{
    (void)sprites;

    // Fill background with dark color
    rend.draw_rect(0, 0, static_cast<int32_t>(rend.width()), static_cast<int32_t>(rend.height()),
                   sf::Color(20, 20, 30), true);

    // Calculate center of screen
    auto screen_width = static_cast<int32_t>(rend.width());
    auto screen_height = static_cast<int32_t>(rend.height());

    // Draw "Connection lost" text centered
    // Using size 24 for main message
    constexpr uint32_t main_text_size = 24;
    const char* main_text = "Connection lost";

    // Approximate text width (roughly 10 pixels per character at size 24)
    int32_t main_text_width = static_cast<int32_t>(std::strlen(main_text)) * 12;
    int32_t main_x = (screen_width - main_text_width) / 2;
    int32_t main_y = screen_height / 2 - 30;

    rend.draw_text_outlined(main_text, main_x, main_y,
                            sf::Color::White, sf::Color::Black,
                            main_text_size, 2.0f);

    // Draw reason if provided (smaller text below)
    if (!reason_.empty())
    {
        constexpr uint32_t reason_text_size = 14;
        int32_t reason_width = static_cast<int32_t>(reason_.size()) * 7;
        int32_t reason_x = (screen_width - reason_width) / 2;
        int32_t reason_y = main_y + 40;

        rend.draw_text_outlined(reason_, reason_x, reason_y,
                                sf::Color(180, 180, 180), sf::Color::Black,
                                reason_text_size, 1.0f);
    }

    // Draw countdown (smaller text at bottom)
    float remaining = timeout_duration_ - elapsed_time_;
    if (remaining < 0.0f) remaining = 0.0f;

    char countdown_text[64];
    std::snprintf(countdown_text, sizeof(countdown_text),
                  "Returning to main menu in %.0f...", std::ceil(remaining));

    constexpr uint32_t countdown_text_size = 14;
    int32_t countdown_width = static_cast<int32_t>(std::strlen(countdown_text)) * 7;
    int32_t countdown_x = (screen_width - countdown_width) / 2;
    int32_t countdown_y = main_y + 80;

    rend.draw_text_outlined(countdown_text, countdown_x, countdown_y,
                            sf::Color(150, 150, 150), sf::Color::Black,
                            countdown_text_size, 1.0f);
}

void connection_lost_screen::render_cursor(renderer& rend, sprite_manager& sprites)
{
    // Draw mouse cursor
    draw_sprite(rend, sprites, 0, mouse_x_, mouse_y_, 0);
}

} // namespace hb
