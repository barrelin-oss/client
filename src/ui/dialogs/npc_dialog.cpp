#include "ui/dialogs/npc_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>

namespace hb
{

// ============ npc_dialog ============

npc_dialog::npc_dialog() : dialog(dialog_type::npc_dialog)
{
    set_title("Dialog");
    set_bounds({static_cast<int32_t>(screen_width) / 2 - 175, static_cast<int32_t>(screen_height) / 2 - 120, 350, 320}); // six options (city hall officer) plus the greeting fit
    set_draggable(true);
    set_closeable(true);
    set_modal(true);
    set_visible(false);
}

void npc_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

void npc_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // NPC portrait area (placeholder)
    rend.draw_rect(x, y, 60, 60, sf::Color(50, 50, 70), true);
    rend.draw_rect(x, y, 60, 60, sf::Color(70, 70, 90), false);

    // NPC name
    rend.draw_text(npc_name_, x + 70, y, sf::Color::Yellow);
    y += 20;

    // Message text (wrap if needed)
    int32_t message_x = x + 70;
    int32_t max_width = bounds_.width - 90;

    // Simple word wrap
    std::string remaining = message_;
    int32_t line_y = y;
    while (!remaining.empty() && line_y < bounds_.y + bounds_.height - 100)
    {
        size_t chars_per_line = static_cast<size_t>(max_width / 6);
        std::string line;

        if (remaining.length() <= chars_per_line)
        {
            line = remaining;
            remaining.clear();
        }
        else
        {
            size_t break_pos = remaining.rfind(' ', chars_per_line);
            if (break_pos == std::string::npos || break_pos == 0)
            {
                break_pos = chars_per_line;
            }
            line = remaining.substr(0, break_pos);
            remaining = remaining.substr(break_pos + 1);
        }

        rend.draw_text(line, message_x, line_y, sf::Color::White, 12);
        line_y += 16;
    }

    // Separator before options
    y = bounds_.y + 130;
    rend.draw_line(x, y, x + 330, y, sf::Color(60, 60, 80));
    y += 10;

    options_start_y_ = y;

    // Dialog options
    for (size_t i = 0; i < options_.size(); ++i)
    {
        const auto& opt = options_[i];

        bool hovered = hovered_option_.has_value() && hovered_option_.value() == i;

        // Option background on hover
        if (hovered && opt.enabled)
        {
            rend.draw_rect(x - 2, y - 2, 334, option_height - 2, sf::Color(50, 60, 80), true);
        }

        // Option bullet
        sf::Color text_color =
            opt.enabled ? (hovered ? sf::Color::Yellow : sf::Color::White) : sf::Color(100, 100, 100);

        rend.draw_text(">", x, y, text_color);
        rend.draw_text(opt.text, x + 16, y, text_color, 12);

        y += option_height;
    }
}

bool npc_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (btn == sf::Mouse::Button::Left && hovered_option_.has_value())
    {
        size_t idx = hovered_option_.value();
        if (idx < options_.size() && options_[idx].enabled)
        {
            if (on_option_select_)
            {
                on_option_select_(options_[idx].id);
            }
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool npc_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    hovered_option_.reset();

    // Check if hovering over an option
    int32_t base_x = bounds_.x + 8;
    if (x >= base_x && x < base_x + 334 && y >= options_start_y_)
    {
        int32_t option_idx = (y - options_start_y_) / option_height;
        if (option_idx >= 0 && static_cast<size_t>(option_idx) < options_.size())
        {
            hovered_option_ = static_cast<size_t>(option_idx);
        }
    }

    return dialog::handle_mouse_move(x, y);
}

void npc_dialog::set_options(const std::vector<npc_option>& options)
{
    options_ = options;
    hovered_option_.reset();
}

void npc_dialog::add_option(const npc_option& option)
{
    options_.push_back(option);
}

void npc_dialog::clear_options()
{
    options_.clear();
    hovered_option_.reset();
}

// ============ quest_dialog ============

} // namespace hb
