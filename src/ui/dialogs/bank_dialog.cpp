#include "ui/dialogs/bank_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>

namespace hb
{

bank_dialog::bank_dialog() : dialog(dialog_type::bank)
{
    set_title("Bank");

    // Calculate dialog size based on grid
    int32_t grid_width = grid_cols * (slot_size + slot_padding) + slot_padding;
    int32_t grid_height = grid_rows * (slot_size + slot_padding) + slot_padding;
    int32_t dialog_width = grid_width + 20;
    int32_t dialog_height = grid_height + 80;

    set_bounds({static_cast<int32_t>(screen_width) / 2 - dialog_width / 2,
                static_cast<int32_t>(screen_height) / 2 - dialog_height / 2,
                dialog_width,
                dialog_height});
    set_draggable(true);
    set_closeable(true);
    set_modal(true);
    set_visible(false);
}

void bank_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

void bank_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t base_x = bounds_.x + 10;
    int32_t base_y = bounds_.y + 32;

    // Gold display
    rend.draw_text(std::format("Stored Gold: {}", stored_gold_), base_x, base_y, sf::Color(255, 220, 100));
    base_y += 24;

    // Draw grid background
    int32_t grid_width = grid_cols * (slot_size + slot_padding) + slot_padding;
    int32_t grid_height = grid_rows * (slot_size + slot_padding) + slot_padding;
    rend.draw_rect(base_x, base_y, grid_width, grid_height, sf::Color(30, 30, 40), true);
    rend.draw_rect(base_x, base_y, grid_width, grid_height, sf::Color(60, 60, 80), false);

    // Draw slots
    for (int32_t row = 0; row < grid_rows; ++row)
    {
        for (int32_t col = 0; col < grid_cols; ++col)
        {
            int32_t slot = row * grid_cols + col;
            int32_t slot_x = base_x + slot_padding + col * (slot_size + slot_padding);
            int32_t slot_y = base_y + slot_padding + row * (slot_size + slot_padding);

            render_slot(rend, slot, slot_x, slot_y);
        }
    }

    // Instructions
    int32_t info_y = base_y + grid_height + 8;
    rend.draw_text("Right-click to withdraw", base_x, info_y, sf::Color(150, 150, 180), 11);
    rend.draw_text("Drag from inventory to deposit", base_x, info_y + 14, sf::Color(150, 150, 180), 11);
}

void bank_dialog::render_slot(renderer& rend, int32_t slot, int32_t x, int32_t y)
{
    // Slot background
    sf::Color bg_color = sf::Color(40, 40, 50);
    if (hovered_slot_.has_value() && hovered_slot_.value() == slot)
    {
        bg_color = sf::Color(60, 60, 80);
    }
    if (is_dragging_ && dragging_slot_.has_value() && dragging_slot_.value() == slot)
    {
        bg_color = sf::Color(80, 80, 60);
    }

    rend.draw_rect(x, y, slot_size, slot_size, bg_color, true);
    rend.draw_rect(x, y, slot_size, slot_size, sf::Color(60, 60, 80), false);

    // Draw item if present
    if (slots_[slot].occupied && slots_[slot].item_ptr)
    {
        const item* itm = slots_[slot].item_ptr;

        // Item color based on type
        sf::Color item_color;
        if (itm->is_weapon())
        {
            item_color = sf::Color(200, 100, 100);
        }
        else if (itm->is_armor())
        {
            item_color = sf::Color(100, 100, 200);
        }
        else if (itm->type == item_type::consume || itm->type == item_type::eat)
        {
            item_color = sf::Color(100, 200, 100);
        }
        else if (itm->type == item_type::apply || itm->type == item_type::use_deplete)
        {
            item_color = sf::Color(200, 200, 100);
        }
        else
        {
            item_color = sf::Color(150, 150, 150);
        }

        rend.draw_rect(x + 3, y + 3, slot_size - 6, slot_size - 6, item_color, true);

        // Draw quantity if stackable
        if (itm->amount > 1)
        {
            std::string count_str = std::to_string(itm->amount);
            rend.draw_text(count_str, x + 2, y + slot_size - 12, sf::Color::White, 9);
        }
    }
}

ui_rect bank_dialog::get_slot_rect(int32_t slot) const
{
    int32_t row = slot / grid_cols;
    int32_t col = slot % grid_cols;

    int32_t base_x = bounds_.x + 10;
    int32_t base_y = bounds_.y + 56;

    return {base_x + slot_padding + col * (slot_size + slot_padding),
            base_y + slot_padding + row * (slot_size + slot_padding),
            slot_size,
            slot_size};
}

std::optional<int32_t> bank_dialog::slot_at(int32_t x, int32_t y) const
{
    for (int32_t i = 0; i < max_slots; ++i)
    {
        ui_rect rect = get_slot_rect(i);
        if (rect.contains(x, y))
        {
            return i;
        }
    }
    return std::nullopt;
}

bool bank_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (auto slot = slot_at(x, y))
    {
        if (btn == sf::Mouse::Button::Left)
        {
            if (slots_[slot.value()].occupied)
            {
                dragging_slot_ = slot;
                drag_start_x_ = x;
                drag_start_y_ = y;
            }
            return true;
        }
        else if (btn == sf::Mouse::Button::Right)
        {
            // Withdraw item
            if (slots_[slot.value()].occupied && on_withdraw_)
            {
                on_withdraw_(slot.value());
            }
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool bank_dialog::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (btn == sf::Mouse::Button::Left && dragging_slot_.has_value())
    {
        if (is_dragging_)
        {
            auto target_slot = slot_at(x, y);
            if (target_slot.has_value() && target_slot.value() != dragging_slot_.value())
            {
                if (on_item_drag_)
                {
                    on_item_drag_(dragging_slot_.value(), target_slot.value());
                }
            }
        }
        else
        {
            // Was a click
            if (on_slot_click_)
            {
                on_slot_click_(dragging_slot_.value());
            }
        }

        dragging_slot_.reset();
        is_dragging_ = false;
        return true;
    }

    return dialog::handle_mouse_up(x, y, btn);
}

bool bank_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    hovered_slot_ = slot_at(x, y);

    // Check if dragging
    if (dragging_slot_.has_value() && !is_dragging_)
    {
        int32_t dx = x - drag_start_x_;
        int32_t dy = y - drag_start_y_;
        if (dx * dx + dy * dy > 25)
        {
            is_dragging_ = true;
        }
    }

    return dialog::handle_mouse_move(x, y);
}

void bank_dialog::set_item(int32_t slot, const item* itm)
{
    if (slot >= 0 && slot < max_slots)
    {
        slots_[slot].item_ptr = itm;
        slots_[slot].occupied = (itm != nullptr);
    }
}

void bank_dialog::clear_slot(int32_t slot)
{
    if (slot >= 0 && slot < max_slots)
    {
        slots_[slot].item_ptr = nullptr;
        slots_[slot].occupied = false;
    }
}

void bank_dialog::clear_all()
{
    for (auto& slot : slots_)
    {
        slot.item_ptr = nullptr;
        slot.occupied = false;
    }
}

} // namespace hb
