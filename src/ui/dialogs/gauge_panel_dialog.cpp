#include "ui/dialogs/gauge_panel_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace hb
{

gauge_panel_dialog::gauge_panel_dialog() : dialog(dialog_type::none)
{ // Custom HUD element
    // Position at top-left of screen
    set_title("");
    set_bounds({10, 10, bar_width + 80, 110});
    set_closeable(false);
    set_draggable(false);
    set_modal(false);
    set_has_border(true);
    set_background_color(sf::Color(20, 20, 30, 200));
}

void gauge_panel_dialog::update(float delta_time, const input& inp)
{
    (void)inp;
    if (!visible_)
        return;

    // Smoothly interpolate display values for visual effect
    float lerp_speed = 5.0f * delta_time;

    float target_hp = max_hp_ > 0 ? static_cast<float>(current_hp_) / max_hp_ : 0.0f;
    float target_mp = max_mp_ > 0 ? static_cast<float>(current_mp_) / max_mp_ : 0.0f;
    float target_sp = max_sp_ > 0 ? static_cast<float>(current_sp_) / max_sp_ : 0.0f;
    float target_exp = exp_to_next_level_ > 0 ? static_cast<float>(current_exp_) / exp_to_next_level_ : 0.0f;

    hp_display_ += (target_hp - hp_display_) * lerp_speed;
    mp_display_ += (target_mp - mp_display_) * lerp_speed;
    sp_display_ += (target_sp - sp_display_) * lerp_speed;
    exp_display_ += (target_exp - exp_display_) * lerp_speed;

    // Clamp values
    hp_display_ = std::clamp(hp_display_, 0.0f, 1.0f);
    mp_display_ = std::clamp(mp_display_, 0.0f, 1.0f);
    sp_display_ = std::clamp(sp_display_, 0.0f, 1.0f);
    exp_display_ = std::clamp(exp_display_, 0.0f, 1.0f);
}

void gauge_panel_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    // Render background panel
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, bg_color_, true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height, border_color_, false);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 8;

    // Player name and level
    std::string player_info = player_name_;
    if (!player_info.empty())
    {
        player_info += " Lv." + std::to_string(player_level_);
        if (is_warrior_)
        {
            player_info += " (W)";
        }
        else
        {
            player_info += " (M)";
        }
        rend.draw_text(player_info, x, y, sf::Color::White, 12);
    }

    y += 18;

    // HP bar
    std::string hp_text = show_values_ ? std::to_string(current_hp_) + "/" + std::to_string(max_hp_) : "";
    render_bar(
        rend, x, y, bar_width, bar_height, hp_display_, sf::Color(200, 50, 50), sf::Color(60, 20, 20), "HP", hp_text);

    y += bar_height + bar_spacing;

    // MP bar
    std::string mp_text = show_values_ ? std::to_string(current_mp_) + "/" + std::to_string(max_mp_) : "";
    render_bar(
        rend, x, y, bar_width, bar_height, mp_display_, sf::Color(50, 100, 200), sf::Color(20, 40, 80), "MP", mp_text);

    y += bar_height + bar_spacing;

    // SP bar
    std::string sp_text = show_values_ ? std::to_string(current_sp_) + "/" + std::to_string(max_sp_) : "";
    render_bar(
        rend, x, y, bar_width, bar_height, sp_display_, sf::Color(200, 180, 50), sf::Color(80, 70, 20), "SP", sp_text);

    y += bar_height + bar_spacing;

    // EXP bar
    std::string exp_percent = show_values_ ? std::to_string(static_cast<int>(exp_display_ * 100)) + "%" : "";
    render_bar(rend,
               x,
               y,
               bar_width,
               bar_height - 2,
               exp_display_,
               sf::Color(50, 200, 100),
               sf::Color(20, 70, 40),
               "EXP",
               exp_percent);
}

void gauge_panel_dialog::render_bar(renderer& rend,
                                    int32_t x,
                                    int32_t y,
                                    int32_t width,
                                    int32_t height,
                                    float percent,
                                    sf::Color fill_color,
                                    sf::Color bg_color,
                                    std::string_view label,
                                    std::string_view value_text)
{
    // Label on the left
    rend.draw_text(label, x, y + 1, sf::Color(180, 180, 200), 11);

    // Bar starts after label
    int32_t actual_bar_x = x + 30;
    int32_t actual_bar_width = width - 30;

    // Background
    rend.draw_rect(actual_bar_x, y, actual_bar_width, height, bg_color, true);

    // Fill
    int32_t fill_width = static_cast<int32_t>(actual_bar_width * percent);
    if (fill_width > 0)
    {
        rend.draw_rect(actual_bar_x, y, fill_width, height, fill_color, true);

        // Highlight at top of fill
        sf::Color highlight(static_cast<uint8_t>(std::min(255, fill_color.r + 40)),
                            static_cast<uint8_t>(std::min(255, fill_color.g + 40)),
                            static_cast<uint8_t>(std::min(255, fill_color.b + 40)));
        rend.draw_rect(actual_bar_x, y, fill_width, 2, highlight, true);
    }

    // Border
    rend.draw_rect(actual_bar_x, y, actual_bar_width, height, sf::Color(80, 80, 100), false);

    // Value text (centered on bar)
    if (!value_text.empty())
    {
        int32_t text_x = actual_bar_x + (actual_bar_width - static_cast<int32_t>(value_text.length()) * 6) / 2;
        rend.draw_text(value_text, text_x, y + 2, sf::Color::White, 10);
    }
}

void gauge_panel_dialog::set_hp(int32_t current, int32_t max)
{
    current_hp_ = std::max(0, current);
    max_hp_ = std::max(1, max);
}

void gauge_panel_dialog::set_mp(int32_t current, int32_t max)
{
    current_mp_ = std::max(0, current);
    max_mp_ = std::max(1, max);
}

void gauge_panel_dialog::set_sp(int32_t current, int32_t max)
{
    current_sp_ = std::max(0, current);
    max_sp_ = std::max(1, max);
}

void gauge_panel_dialog::set_exp(int64_t current, int64_t next_level)
{
    current_exp_ = std::max(static_cast<int64_t>(0), current);
    exp_to_next_level_ = std::max(static_cast<int64_t>(1), next_level);
}

void gauge_panel_dialog::set_player_name(std::string_view name)
{
    player_name_ = name;
}

void gauge_panel_dialog::set_player_level(uint16_t level)
{
    player_level_ = level;
}

void gauge_panel_dialog::set_player_class(bool warrior)
{
    is_warrior_ = warrior;
}

} // namespace hb
