#include "ui/dialogs/fishing_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <format>
#include <spdlog/spdlog.h>

namespace hb {

fishing_dialog::fishing_dialog()
    : dialog(dialog_type::fishing)
{
    set_title("Fishing");
    set_bounds({
        static_cast<int32_t>(screen_width) / 2 - dialog_width / 2,
        static_cast<int32_t>(screen_height) / 2 - dialog_height / 2,
        dialog_width,
        dialog_height
    });
    set_modal(true);
    set_closeable(false);
    set_draggable(true);
}

void fishing_dialog::open_fishing(std::string_view fish_name, uint8_t visual_type, int32_t catch_chance)
{
    fish_name_ = fish_name;
    visual_type_ = visual_type;
    catch_chance_ = catch_chance;
    open();
    spdlog::info("Fishing engaged: {} (type {}, chance {}%)", fish_name_, visual_type_, catch_chance_);
}

void fishing_dialog::update_catch_chance(int32_t chance)
{
    catch_chance_ = std::clamp(chance, 0, 100);
}

void fishing_dialog::close_fishing()
{
    close();
    fish_name_.clear();
    visual_type_ = 0;
    catch_chance_ = 0;
}

void fishing_dialog::update(float delta_time, const input& inp)
{
    if (!visible_) return;

    dialog::update(delta_time, inp);

    // Hit test for hover
    hovered_element_ = -1;

    int32_t btn_width = 90;
    int32_t btn_height = 28;
    int32_t btn_y = bounds_.y + dialog_height - 45;
    int32_t catch_x = bounds_.x + dialog_width / 2 - btn_width - 10;
    int32_t cancel_x = bounds_.x + dialog_width / 2 + 10;

    int32_t mx = inp.mouse_x();
    int32_t my = inp.mouse_y();

    if (mx >= catch_x && mx < catch_x + btn_width && my >= btn_y && my < btn_y + btn_height)
        hovered_element_ = elem_catch_button;
    else if (mx >= cancel_x && mx < cancel_x + btn_width && my >= btn_y && my < btn_y + btn_height)
        hovered_element_ = elem_cancel_button;
}

void fishing_dialog::render(renderer& rend)
{
    if (!visible_) return;

    // Dialog background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(30, 30, 45, 245), true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(80, 80, 100), false);

    // Title bar
    int32_t tb_h = 24;
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, tb_h,
                   sf::Color(50, 50, 70), true);
    rend.draw_text(title_, bounds_.x + 10, bounds_.y + 6, sf::Color::White, 12);

    int32_t content_y = bounds_.y + tb_h + 15;
    int32_t center_x = bounds_.x + dialog_width / 2;

    // Fish name
    int32_t name_w = static_cast<int32_t>(fish_name_.size()) * 8;
    rend.draw_text(fish_name_, center_x - name_w / 2, content_y,
                   sf::Color(220, 200, 100), 14);
    content_y += 28;

    // Catch chance bar background
    int32_t bar_x = bounds_.x + 20;
    int32_t bar_width = dialog_width - 40;
    int32_t bar_height = 20;

    rend.draw_rect(bar_x, content_y, bar_width, bar_height,
                   sf::Color(35, 35, 50), true);
    rend.draw_rect(bar_x, content_y, bar_width, bar_height,
                   sf::Color(80, 80, 100), false);

    // Catch chance fill
    int32_t fill_width = bar_width * catch_chance_ / 100;
    if (fill_width > 0)
    {
        // Color based on catch chance: red -> yellow -> green
        sf::Color fill_color;
        if (catch_chance_ < 30)
            fill_color = sf::Color(180, 60, 60);
        else if (catch_chance_ < 60)
            fill_color = sf::Color(200, 180, 50);
        else
            fill_color = sf::Color(60, 180, 60);

        rend.draw_rect(bar_x + 1, content_y + 1, fill_width - 2, bar_height - 2,
                       fill_color, true);
    }

    // Catch chance text
    std::string chance_text = std::to_string(catch_chance_) + "%";
    int32_t text_w = static_cast<int32_t>(chance_text.size()) * 7;
    rend.draw_text(chance_text, center_x - text_w / 2, content_y + 3,
                   sf::Color::White, 12);

    content_y += bar_height + 8;

    // "Catch Chance" label
    std::string label = "Catch Chance";
    int32_t label_w = static_cast<int32_t>(label.size()) * 6;
    rend.draw_text(label, center_x - label_w / 2, content_y,
                   sf::Color(150, 150, 180), 10);

    // Buttons
    int32_t btn_width = 90;
    int32_t btn_height = 28;
    int32_t btn_y = bounds_.y + dialog_height - 45;
    int32_t catch_x = bounds_.x + dialog_width / 2 - btn_width - 10;
    int32_t cancel_x = bounds_.x + dialog_width / 2 + 10;

    // Catch button
    bool catch_hovered = (hovered_element_ == elem_catch_button);
    sf::Color catch_bg = catch_hovered ? sf::Color(70, 120, 70) : sf::Color(50, 90, 50);
    rend.draw_rect(catch_x, btn_y, btn_width, btn_height, catch_bg, true);
    rend.draw_rect(catch_x, btn_y, btn_width, btn_height, sf::Color(100, 160, 100), false);
    int32_t catch_tw = 5 * 7; // "Catch"
    rend.draw_text("Catch", catch_x + (btn_width - catch_tw) / 2 + 1, btn_y + 8 + 1,
                   sf::Color(0, 0, 0), 12);
    rend.draw_text("Catch", catch_x + (btn_width - catch_tw) / 2, btn_y + 8,
                   catch_hovered ? sf::Color(255, 255, 200) : sf::Color::White, 12);

    // Cancel button
    bool cancel_hovered = (hovered_element_ == elem_cancel_button);
    sf::Color cancel_bg = cancel_hovered ? sf::Color(120, 60, 60) : sf::Color(90, 50, 50);
    rend.draw_rect(cancel_x, btn_y, btn_width, btn_height, cancel_bg, true);
    rend.draw_rect(cancel_x, btn_y, btn_width, btn_height, sf::Color(160, 100, 100), false);
    int32_t cancel_tw = 6 * 7; // "Cancel"
    rend.draw_text("Cancel", cancel_x + (btn_width - cancel_tw) / 2 + 1, btn_y + 8 + 1,
                   sf::Color(0, 0, 0), 12);
    rend.draw_text("Cancel", cancel_x + (btn_width - cancel_tw) / 2, btn_y + 8,
                   cancel_hovered ? sf::Color(255, 255, 200) : sf::Color::White, 12);
}

bool fishing_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_) return false;

    if (btn == sf::Mouse::Button::Left)
    {
        int32_t btn_width = 90;
        int32_t btn_height = 28;
        int32_t btn_y = bounds_.y + dialog_height - 45;
        int32_t catch_x = bounds_.x + dialog_width / 2 - btn_width - 10;
        int32_t cancel_x = bounds_.x + dialog_width / 2 + 10;

        if (x >= catch_x && x < catch_x + btn_width && y >= btn_y && y < btn_y + btn_height)
        {
            if (on_catch_) on_catch_();
            return true;
        }

        if (x >= cancel_x && x < cancel_x + btn_width && y >= btn_y && y < btn_y + btn_height)
        {
            close_fishing();
            if (on_cancel_) on_cancel_();
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

} // namespace hb
