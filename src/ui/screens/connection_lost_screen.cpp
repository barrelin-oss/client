#include "ui/screens/connection_lost_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void connection_lost_screen::on_enter()
{
    elapsed_time_ = 0.0f;
    dismissed_ = false;
    spdlog::info("Connection lost screen entered");
}

void connection_lost_screen::on_exit()
{
    reason_.clear();
    spdlog::info("Connection lost screen exited");
}

bool connection_lost_screen::update(float delta_time, const input& inp)
{
    elapsed_time_ += delta_time;

    // Store mouse position for cursor rendering
    mouse_x_ = inp.mouse_x();
    mouse_y_ = inp.mouse_y();

    // Enter, Escape, or Space to dismiss immediately
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter) || inp.is_key_pressed(sf::Keyboard::Key::Escape) ||
        inp.is_key_pressed(sf::Keyboard::Key::Space))
    {
        dismiss();
        return true;
    }

    // Check Ok button click
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left) && screen_width_ > 0)
    {
        int32_t btn_x = (screen_width_ - button_width_) / 2;
        int32_t btn_y = screen_height_ / 2 + 70;

        if (mouse_x_ >= btn_x && mouse_x_ <= btn_x + button_width_ && mouse_y_ >= btn_y &&
            mouse_y_ <= btn_y + button_height_)
        {
            play_button_sound();
            dismiss();
            return true;
        }
    }

    if (elapsed_time_ >= timeout_duration_)
    {
        dismiss();
    }

    return true;
}

void connection_lost_screen::dismiss()
{
    if (dismissed_)
        return;
    dismissed_ = true;

    if (on_timeout_)
    {
        on_timeout_();
    }
}

void connection_lost_screen::render(renderer& rend, sprite_manager& sprites)
{
    (void)sprites;

    // Cache screen size for update() hit testing
    screen_width_ = static_cast<int32_t>(rend.width());
    screen_height_ = static_cast<int32_t>(rend.height());

    auto screen_width = screen_width_;
    auto screen_height = screen_height_;

    // Fill background with dark color
    rend.draw_rect(0, 0, screen_width, screen_height, sf::Color(20, 20, 30), true);

    // Draw "Connection lost" text centered
    // Using size 24 for main message
    constexpr uint32_t main_text_size = 24;
    const char* main_text = "Connection lost";

    // Approximate text width (roughly 10 pixels per character at size 24)
    int32_t main_text_width = static_cast<int32_t>(std::strlen(main_text)) * 12;
    int32_t main_x = (screen_width - main_text_width) / 2;
    int32_t main_y = screen_height / 2 - 30;

    rend.draw_text_outlined(main_text, main_x, main_y, sf::Color::White, sf::Color::Black, main_text_size, 2.0f);

    // Draw reason if provided (smaller text below)
    if (!reason_.empty())
    {
        constexpr uint32_t reason_text_size = 14;
        int32_t reason_width = static_cast<int32_t>(reason_.size()) * 7;
        int32_t reason_x = (screen_width - reason_width) / 2;
        int32_t reason_y = main_y + 40;

        rend.draw_text_outlined(
            reason_, reason_x, reason_y, sf::Color(180, 180, 180), sf::Color::Black, reason_text_size, 1.0f);
    }

    // Draw countdown (smaller text at bottom)
    float remaining = timeout_duration_ - elapsed_time_;
    if (remaining < 0.0f)
        remaining = 0.0f;

    char countdown_text[64];
    std::snprintf(countdown_text, sizeof(countdown_text), "Returning to main menu in %.0f...", std::ceil(remaining));

    constexpr uint32_t countdown_text_size = 14;
    int32_t countdown_width = static_cast<int32_t>(std::strlen(countdown_text)) * 7;
    int32_t countdown_x = (screen_width - countdown_width) / 2;
    int32_t countdown_y = main_y + 80;

    rend.draw_text_outlined(countdown_text,
                            countdown_x,
                            countdown_y,
                            sf::Color(150, 150, 150),
                            sf::Color::Black,
                            countdown_text_size,
                            1.0f);

    // Draw Ok button
    int32_t btn_x = (screen_width - button_width_) / 2;
    int32_t btn_y = main_y + 100;

    bool hovered = mouse_x_ >= btn_x && mouse_x_ <= btn_x + button_width_ && mouse_y_ >= btn_y &&
                   mouse_y_ <= btn_y + button_height_;

    sf::Color btn_bg = hovered ? sf::Color(80, 80, 110) : sf::Color(50, 50, 70);
    sf::Color btn_border = hovered ? sf::Color(140, 140, 180) : sf::Color(100, 100, 130);

    rend.draw_rect(btn_x, btn_y, button_width_, button_height_, btn_bg, true);
    rend.draw_rect(btn_x, btn_y, button_width_, button_height_, btn_border, false);

    // Center "Ok" text within button
    constexpr uint32_t btn_text_size = 14;
    int32_t text_w = 2 * 7; // "Ok" = 2 chars
    int32_t text_x = btn_x + (button_width_ - text_w) / 2;
    int32_t text_y = btn_y + (button_height_ - static_cast<int32_t>(btn_text_size)) / 2;

    rend.draw_text("Ok", text_x, text_y, sf::Color::White, btn_text_size);
}

} // namespace hb
