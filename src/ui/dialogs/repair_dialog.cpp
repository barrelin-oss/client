#include "ui/dialogs/repair_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>
#include <algorithm>

namespace hb
{

repair_dialog::repair_dialog() : dialog(dialog_type::repair)
{
    set_title("Repair Items");
    set_bounds({180, 100, 300, 340});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void repair_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

void repair_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // NPC name
    if (!npc_name_.empty())
    {
        rend.draw_text(npc_name_, x, y, sf::Color(200, 200, 100));
        y += 20;
    }

    // Player gold
    rend.draw_text(std::format("Your Gold: {}", player_gold_), x, y, sf::Color(255, 220, 100));
    y += 22;

    // Separator
    rend.draw_line(x, y, x + 280, y, sf::Color(80, 80, 100));
    y += 8;

    // Item list header
    rend.draw_text("Damaged Items:", x, y, sf::Color::White);
    y += 18;

    if (repair_items_.empty())
    {
        rend.draw_text("No items need repair.", x, y, sf::Color(150, 150, 150));
    }
    else
    {
        list_area_x_ = x;
        list_area_y_ = y;
        render_item_list(rend, x, y);
        y += visible_items * item_row_height + 10;

        // Selected item details
        if (selected_item_ >= 0 && selected_item_ < static_cast<int32_t>(repair_items_.size()))
        {
            rend.draw_line(x, y, x + 280, y, sf::Color(80, 80, 100));
            y += 8;
            render_selected_info(rend, x, y);
        }
    }

    // Buttons
    int32_t btn_y = bounds_.y + bounds_.height - 45;

    // Repair selected button
    bool can_repair_selected = selected_item_ >= 0 && selected_item_ < static_cast<int32_t>(repair_items_.size()) &&
                               player_gold_ >= repair_items_[selected_item_].repair_cost;
    sf::Color repair_btn_color = can_repair_selected ? sf::Color(60, 100, 60) : sf::Color(60, 60, 60);

    rend.draw_rect(x, btn_y, 100, 28, repair_btn_color, true);
    rend.draw_rect(x, btn_y, 100, 28, sf::Color(100, 100, 120), false);
    rend.draw_text("Repair", x + 28, btn_y + 6, sf::Color::White);

    // Repair all button
    uint32_t total_cost = calculate_total_repair_cost();
    bool can_repair_all = !repair_items_.empty() && player_gold_ >= total_cost;
    sf::Color repair_all_color = can_repair_all ? sf::Color(60, 80, 100) : sf::Color(60, 60, 60);

    rend.draw_rect(x + 120, btn_y, 100, 28, repair_all_color, true);
    rend.draw_rect(x + 120, btn_y, 100, 28, sf::Color(100, 100, 120), false);
    rend.draw_text("Repair All", x + 135, btn_y + 6, sf::Color::White);

    // Total cost for repair all
    if (!repair_items_.empty())
    {
        std::string total_text = std::format("Total: {} gold", total_cost);
        sf::Color cost_color = can_repair_all ? sf::Color(200, 200, 100) : sf::Color(200, 100, 100);
        rend.draw_text(total_text, x + 120, btn_y - 16, cost_color, 11);
    }
}

void repair_dialog::render_item_list(renderer& rend, int32_t x, int32_t y)
{
    int32_t list_width = 270;

    for (int32_t i = 0; i < visible_items; ++i)
    {
        int32_t item_idx = scroll_offset_ + i;
        if (item_idx >= static_cast<int32_t>(repair_items_.size()))
            break;

        const auto& info = repair_items_[item_idx];
        int32_t row_y = y + i * item_row_height;

        bool hovered = hovered_item_.has_value() && hovered_item_.value() == item_idx;
        bool selected = selected_item_ == item_idx;

        sf::Color bg_color;
        if (selected)
        {
            bg_color = sf::Color(60, 80, 100);
        }
        else if (hovered)
        {
            bg_color = sf::Color(50, 50, 65);
        }
        else
        {
            bg_color = sf::Color(40, 40, 50);
        }

        rend.draw_rect(x, row_y, list_width, item_row_height - 2, bg_color, true);

        // Item icon placeholder
        sf::Color item_color(150, 100, 100);
        rend.draw_rect(x + 4, row_y + 4, 24, 24, item_color, true);

        // Item name
        std::string item_name = info.itm ? info.itm->name : "Unknown Item";
        rend.draw_text(item_name, x + 34, row_y + 4, sf::Color::White, 11);

        // Durability bar
        int32_t bar_x = x + 34;
        int32_t bar_y = row_y + 18;
        int32_t bar_width = 100;
        int32_t bar_height = 6;

        float dur_ratio =
            info.max_durability > 0 ? static_cast<float>(info.current_durability) / info.max_durability : 0.0f;

        sf::Color bar_fill;
        if (dur_ratio > 0.5f)
        {
            bar_fill = sf::Color(100, 200, 100);
        }
        else if (dur_ratio > 0.25f)
        {
            bar_fill = sf::Color(200, 200, 100);
        }
        else
        {
            bar_fill = sf::Color(200, 100, 100);
        }

        rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(30, 30, 40), true);
        rend.draw_rect(bar_x, bar_y, static_cast<int32_t>(bar_width * dur_ratio), bar_height, bar_fill, true);

        // Durability text
        std::string dur_text = std::format("{}/{}", info.current_durability, info.max_durability);
        rend.draw_text(dur_text, bar_x + bar_width + 4, bar_y - 2, sf::Color(180, 180, 180), 9);

        // Repair cost
        bool can_afford = player_gold_ >= info.repair_cost;
        sf::Color cost_color = can_afford ? sf::Color(255, 220, 100) : sf::Color(200, 100, 100);
        rend.draw_text(std::format("{} g", info.repair_cost), x + list_width - 50, row_y + 10, cost_color, 11);
    }

    // Scroll indicator
    if (repair_items_.size() > static_cast<size_t>(visible_items))
    {
        int32_t scroll_x = x + list_width + 4;
        int32_t scroll_y = y;
        int32_t scroll_height = visible_items * item_row_height;

        rend.draw_rect(scroll_x, scroll_y, 6, scroll_height, sf::Color(30, 30, 40), true);

        float visible_ratio = static_cast<float>(visible_items) / repair_items_.size();
        int32_t thumb_height = std::max(20, static_cast<int32_t>(scroll_height * visible_ratio));

        float scroll_ratio = static_cast<float>(scroll_offset_) / (repair_items_.size() - visible_items);
        int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_height - thumb_height) * scroll_ratio);

        rend.draw_rect(scroll_x, thumb_y, 6, thumb_height, sf::Color(80, 80, 100), true);
    }
}

void repair_dialog::render_selected_info(renderer& rend, int32_t x, int32_t y)
{
    if (selected_item_ < 0 || selected_item_ >= static_cast<int32_t>(repair_items_.size()))
        return;

    const auto& info = repair_items_[selected_item_];

    // Item name
    std::string item_name = info.itm ? info.itm->name : "Unknown Item";
    rend.draw_text(item_name, x, y, sf::Color(200, 200, 255));
    y += 16;

    // Durability
    std::string dur_text = std::format("Durability: {}/{}", info.current_durability, info.max_durability);
    rend.draw_text(dur_text, x, y, sf::Color(180, 180, 180), 11);
    y += 14;

    // Repair cost
    bool can_afford = player_gold_ >= info.repair_cost;
    sf::Color cost_color = can_afford ? sf::Color(255, 220, 100) : sf::Color(200, 100, 100);
    rend.draw_text(std::format("Repair Cost: {} gold", info.repair_cost), x, y, cost_color, 11);
}

std::optional<int32_t> repair_dialog::item_at(int32_t mx, int32_t my) const
{
    int32_t list_width = 270;

    for (int32_t i = 0; i < visible_items; ++i)
    {
        int32_t item_idx = scroll_offset_ + i;
        if (item_idx >= static_cast<int32_t>(repair_items_.size()))
            break;

        int32_t row_y = list_area_y_ + i * item_row_height;
        ui_rect rect{list_area_x_, row_y, list_width, item_row_height - 2};

        if (rect.contains(mx, my))
        {
            return item_idx;
        }
    }
    return std::nullopt;
}

uint32_t repair_dialog::calculate_total_repair_cost() const
{
    uint32_t total = 0;
    for (const auto& info : repair_items_)
    {
        total += info.repair_cost;
    }
    return total;
}

bool repair_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    int32_t base_x = bounds_.x + 10;
    int32_t btn_y = bounds_.y + bounds_.height - 45;

    // Repair selected button
    ui_rect repair_btn{base_x, btn_y, 100, 28};
    if (repair_btn.contains(x, y) && btn == sf::Mouse::Button::Left)
    {
        if (selected_item_ >= 0 && selected_item_ < static_cast<int32_t>(repair_items_.size()))
        {
            const auto& info = repair_items_[selected_item_];
            if (player_gold_ >= info.repair_cost && on_repair_)
            {
                on_repair_(info.inventory_slot);
            }
        }
        return true;
    }

    // Repair all button
    ui_rect repair_all_btn{base_x + 120, btn_y, 100, 28};
    if (repair_all_btn.contains(x, y) && btn == sf::Mouse::Button::Left)
    {
        uint32_t total_cost = calculate_total_repair_cost();
        if (!repair_items_.empty() && player_gold_ >= total_cost && on_repair_all_)
        {
            on_repair_all_();
        }
        return true;
    }

    // Item list click
    auto item_idx = item_at(x, y);
    if (item_idx.has_value() && btn == sf::Mouse::Button::Left)
    {
        select_item(item_idx.value());
        return true;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool repair_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    hovered_item_ = item_at(x, y);

    return dialog::handle_mouse_move(x, y);
}

void repair_dialog::select_item(int32_t index)
{
    if (index >= 0 && index < static_cast<int32_t>(repair_items_.size()))
    {
        selected_item_ = index;
    }
}

} // namespace hb
