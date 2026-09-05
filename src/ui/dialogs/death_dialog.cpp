#include "ui/dialogs/death_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

death_dialog::death_dialog() : dialog(dialog_type::death)
{
    set_title("");
    set_bounds({0, 0, dialog_width, dialog_height});
    set_closeable(false);
    set_draggable(false);
    set_modal(true);
}

void death_dialog::set_death_info(std::string_view killer_name, bool is_pvp, int32_t xp_lost)
{
    killer_name_ = killer_name;
    is_pvp_ = is_pvp;
    xp_lost_ = xp_lost;
}

void death_dialog::show_resurrect_option(bool show)
{
    resurrect_available_ = show;
}

void death_dialog::update(float /*delta_time*/, const input& inp)
{
    if (!is_open())
        return;

    int32_t mx = inp.mouse_x();
    int32_t my = inp.mouse_y();

    restart_hovered_ = is_point_in_restart_button(mx, my);
    resurrect_hovered_ = resurrect_available_ && is_point_in_resurrect_button(mx, my);
}

void death_dialog::render(renderer& rend)
{
    if (!is_open())
        return;

    // Center horizontally, near top of screen
    int32_t display_w = static_cast<int32_t>(rend.width()); // logical width: the UI view may be scaled
    actual_x_ = (display_w - dialog_width) / 2;
    actual_y_ = top_margin;

    // Background
    rend.draw_rect(actual_x_, actual_y_, dialog_width, dialog_height, sf::Color(20, 10, 10, 220), true);

    // Border
    rend.draw_rect(actual_x_, actual_y_, dialog_width, dialog_height, sf::Color(120, 40, 40), false);
    rend.draw_rect(actual_x_ + 1, actual_y_ + 1, dialog_width - 2, dialog_height - 2, sf::Color(80, 30, 30), false);

    sf::Color outline(0, 0, 0);

    // Title
    const char* title = "You have died";
    int32_t title_w = static_cast<int32_t>(std::strlen(title)) * 7;
    rend.draw_text_outlined(
        title, actual_x_ + (dialog_width - title_w) / 2, actual_y_ + 12, sf::Color(255, 80, 80), outline, 14, 1.0f);

    // Killer info
    int32_t info_y = actual_y_ + 38;
    if (!killer_name_.empty())
    {
        std::string line = is_pvp_ ? "Killed by: " : "Slain by: ";
        line += killer_name_;
        int32_t line_w = static_cast<int32_t>(line.length()) * 6;
        rend.draw_text_outlined(
            line, actual_x_ + (dialog_width - line_w) / 2, info_y, sf::Color(220, 200, 180), outline, 12, 1.0f);
        info_y += 18;
    }

    if (xp_lost_ > 0)
    {
        std::string line = "EXP lost: " + std::to_string(xp_lost_);
        int32_t line_w = static_cast<int32_t>(line.length()) * 6;
        rend.draw_text_outlined(
            line, actual_x_ + (dialog_width - line_w) / 2, info_y, sf::Color(220, 200, 180), outline, 12, 1.0f);
    }

    // Buttons
    int32_t buttons_y = actual_y_ + dialog_height - button_height - 14;

    if (resurrect_available_)
    {
        // Two buttons side by side
        int32_t total_w = button_width * 2 + button_spacing;
        int32_t start_x = actual_x_ + (dialog_width - total_w) / 2;

        // Restart button
        {
            int32_t bx = start_x;
            sf::Color bg = restart_hovered_ ? sf::Color(80, 40, 40) : sf::Color(50, 25, 25);
            rend.draw_rect(bx, buttons_y, button_width, button_height, bg, true);
            rend.draw_rect(bx, buttons_y, button_width, button_height, sf::Color(120, 60, 60), false);

            const char* text = "Restart";
            int32_t tw = static_cast<int32_t>(std::strlen(text)) * 7;
            rend.draw_text_outlined(
                text, bx + (button_width - tw) / 2, buttons_y + 5, sf::Color::White, outline, 12, 1.0f);
        }

        // Resurrect button
        {
            int32_t bx = start_x + button_width + button_spacing;
            sf::Color bg = resurrect_hovered_ ? sf::Color(40, 80, 40) : sf::Color(25, 50, 25);
            rend.draw_rect(bx, buttons_y, button_width, button_height, bg, true);
            rend.draw_rect(bx, buttons_y, button_width, button_height, sf::Color(60, 120, 60), false);

            const char* text = "Resurrect";
            int32_t tw = static_cast<int32_t>(std::strlen(text)) * 7;
            rend.draw_text_outlined(
                text, bx + (button_width - tw) / 2, buttons_y + 5, sf::Color(150, 255, 150), outline, 12, 1.0f);
        }
    }
    else
    {
        // Single centered restart button
        int32_t bx = actual_x_ + (dialog_width - button_width) / 2;
        sf::Color bg = restart_hovered_ ? sf::Color(80, 40, 40) : sf::Color(50, 25, 25);
        rend.draw_rect(bx, buttons_y, button_width, button_height, bg, true);
        rend.draw_rect(bx, buttons_y, button_width, button_height, sf::Color(120, 60, 60), false);

        const char* text = "Restart";
        int32_t tw = static_cast<int32_t>(std::strlen(text)) * 7;
        rend.draw_text_outlined(text, bx + (button_width - tw) / 2, buttons_y + 5, sf::Color::White, outline, 12, 1.0f);
    }
}

bool death_dialog::handle_key_press(sf::Keyboard::Key key)
{
    if (!is_open())
        return false;

    if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
    {
        if (on_restart_)
            on_restart_();
        return true;
    }

    return true; // Consume all keys while dead
}

bool death_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!is_open() || btn != sf::Mouse::Button::Left)
        return false;

    if (is_point_in_restart_button(x, y))
    {
        if (on_restart_)
            on_restart_();
        return true;
    }

    if (resurrect_available_ && is_point_in_resurrect_button(x, y))
    {
        if (on_resurrect_)
            on_resurrect_();
        return true;
    }

    // Consume click even if not on a button (modal)
    return true;
}

bool death_dialog::is_point_in_restart_button(int32_t x, int32_t y) const
{
    int32_t buttons_y = actual_y_ + dialog_height - button_height - 14;
    int32_t bx;

    if (resurrect_available_)
    {
        int32_t total_w = button_width * 2 + button_spacing;
        bx = actual_x_ + (dialog_width - total_w) / 2;
    }
    else
    {
        bx = actual_x_ + (dialog_width - button_width) / 2;
    }

    return x >= bx && x < bx + button_width && y >= buttons_y && y < buttons_y + button_height;
}

bool death_dialog::is_point_in_resurrect_button(int32_t x, int32_t y) const
{
    if (!resurrect_available_)
        return false;

    int32_t buttons_y = actual_y_ + dialog_height - button_height - 14;
    int32_t total_w = button_width * 2 + button_spacing;
    int32_t bx = actual_x_ + (dialog_width - total_w) / 2 + button_width + button_spacing;

    return x >= bx && x < bx + button_width && y >= buttons_y && y < buttons_y + button_height;
}

} // namespace hb
