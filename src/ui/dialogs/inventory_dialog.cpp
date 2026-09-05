#include "ui/dialogs/inventory_dialog.hpp"
#include <spdlog/spdlog.h>
#include "assets/sprite_manager.hpp"
#include "assets/sprite.hpp"
#include "gameplay/inventory.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "world/ground_item.hpp"
#include "core/constants.hpp"
#include <format>

namespace hb
{

inventory_dialog::inventory_dialog() : dialog(dialog_type::inventory)
{
    set_title("Inventory");

    int32_t dialog_width = min_width;
    int32_t dialog_height = min_height;

    set_bounds({static_cast<int32_t>(screen_width) - dialog_width - 10, 100, dialog_width, dialog_height});
    set_draggable(true);
    set_drag_anywhere(true);
    set_closeable(true);
    set_visible(false);
    set_drag_clamp(drag_clamp::partial);
}

ui_rect inventory_dialog::bag_area() const
{
    return {bounds_.x + chrome_sides, bounds_.y + chrome_top,
            bounds_.width - chrome_sides * 2, bounds_.height - chrome_top - chrome_bottom};
}

void inventory_dialog::update(float delta_time, const input& /*inp*/)
{
    click_elapsed_ += delta_time;
}

void inventory_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    auto bag = bag_area();

    // Bag background
    rend.draw_rect(bag.x, bag.y, bag.width, bag.height, sf::Color(20, 20, 30), true);
    rend.draw_rect(bag.x, bag.y, bag.width, bag.height, sf::Color(50, 50, 70), false);

    // Render items in z-order (render_order_ is sorted ascending, lower z = drawn first).
    // Dragged item renders at its current (moving) position — no separate copy.
    if (inventory_)
    {
        for (uint32_t item_id : inventory_->render_order())
        {
            const auto* entry = inventory_->get_bag_item(item_id);
            if (!entry)
                continue;

            if (inventory_->equipment().find_slot_for(item_id).has_value())
                continue; // equipped items are shown in the equipment dialog, not here

            render_item(rend, *entry, bag.x + entry->pos_x, bag.y + entry->pos_y);
        }
    }

    // Footer: gold, item count, and weight
    int32_t footer_y = bounds_.y + bounds_.height - chrome_bottom + 4;
    if (inventory_)
    {
        rend.draw_text(std::format("Gold: {}", inventory_->gold()),
                       bounds_.x + chrome_sides, footer_y, sf::Color::Yellow);

        // Item count with color coding
        size_t item_count = inventory_->bag_count();
        sf::Color count_color;
        if (item_count >= max_items)
            count_color = sf::Color::Red;
        else if (item_count > max_items / 2)
            count_color = sf::Color::Yellow;
        else
            count_color = sf::Color::Green;
        rend.draw_text(std::format("{}/{}", item_count, max_items),
                       bounds_.x + bounds_.width - 140, footer_y, count_color);

        // Weight (divided by 100)
        auto wt_color = inventory_->is_overweight() ? sf::Color::Red : sf::Color(180, 180, 180);
        rend.draw_text(std::format("{}/{}", inventory_->current_weight() / 100, inventory_->max_weight() / 100),
                       bounds_.x + bounds_.width - 80, footer_y, wt_color);
    }

    // Resize handle — diagonal grip lines at bottom-right corner
    {
        int32_t bx = bounds_.x + bounds_.width;
        int32_t by = bounds_.y + bounds_.height;
        sf::Color light(160, 160, 190);
        sf::Color dark(50, 50, 70);
        for (int32_t i = 0; i < 3; ++i)
        {
            int32_t off = 3 + i * 3;
            rend.draw_line(bx - off, by - 1, bx - 1, by - off, dark);
            rend.draw_line(bx - off - 1, by - 1, bx - 1, by - off - 1, light);
        }
    }
}

void inventory_dialog::render_item(renderer& rend, const bag_item& entry, int32_t x, int32_t y)
{
    if (!inventory_)
        return;

    const auto& itm = entry.data;
    bool is_locked = entry.locked;

    // Get sprite from item-pack.pak
    bool drew_sprite = false;
    if (sprite_mgr_)
    {
        auto* spr = sprite_mgr_->get_sprite("item-pack", itm.sprite_id - 1);
        if (spr)
        {
            uint32_t frame = static_cast<uint32_t>(itm.sprite_frame);
            if (is_locked)
            {
                rend.draw_sprite_alpha(*spr, x, y, frame, 0.35f);
            }
            else if (itm.color == 0)
            {
                rend.draw_sprite(*spr, x, y, frame);
            }
            else
            {
                float r = 0.0f, g = 0.0f, b = 0.0f;
                rend.draw_sprite_tinted(*spr, x, y, frame, r, g, b);
            }
            drew_sprite = true;
        }
    }

    // Fallback: colored rectangle if no sprite
    if (!drew_sprite)
    {
        sf::Color fallback(100, 100, 150);
        if (itm.is_weapon())
            fallback = sf::Color(200, 100, 100);
        else if (itm.is_armor())
            fallback = sf::Color(100, 100, 200);
        else if (itm.type == item_type::consumable)
            fallback = sf::Color(100, 200, 100);
        if (is_locked)
            fallback.a = 90;
        rend.draw_rect(x + 2, y + 2, 30, 30, fallback, true);
    }

    // Stack count (bottom-right)
    if (itm.count > 1)
    {
        rend.draw_text(std::to_string(itm.count), x + 20, y + 22, sf::Color::White, 10);
    }

    // Durability bar (bottom of item)
    if (itm.max_durability > 0 && itm.durability < itm.max_durability)
    {
        float pct = static_cast<float>(itm.durability) / static_cast<float>(itm.max_durability);
        int32_t bar_w = static_cast<int32_t>(30.0f * pct);
        auto bar_color = pct > 0.5f  ? sf::Color(80, 200, 80)
                         : pct > 0.2f ? sf::Color(200, 200, 80)
                                      : sf::Color(200, 80, 80);
        rend.draw_rect(x + 2, y + 30, bar_w, 2, bar_color, true);
    }
}

ui_rect inventory_dialog::item_sprite_rect(uint32_t item_id) const
{
    auto bag = bag_area();
    const auto* entry = inventory_->get_bag_item(item_id);
    if (!entry)
        return {bag.x, bag.y, cell_size, cell_size};

    // Try to get actual sprite bounds (position + pivot + frame size)
    if (sprite_mgr_)
    {
        auto* spr = sprite_mgr_->get_sprite("item-pack", entry->data.sprite_id - 1);
        if (spr)
        {
            uint32_t frame = static_cast<uint32_t>(entry->data.sprite_frame);
            const auto& f = spr->get_frame(frame);
            return {bag.x + entry->pos_x + f.pivot_x,
                    bag.y + entry->pos_y + f.pivot_y,
                    f.source_rect.size.x,
                    f.source_rect.size.y};
        }
    }

    // Fallback: cell_size box at item position
    return {bag.x + entry->pos_x, bag.y + entry->pos_y, cell_size, cell_size};
}

std::optional<uint32_t> inventory_dialog::item_at(int32_t screen_x, int32_t screen_y) const
{
    if (!inventory_)
        return std::nullopt;

    auto bag = bag_area();
    if (!bag.contains(screen_x, screen_y))
        return std::nullopt;

    // Check each item using pixel-level sprite collision (reverse order = higher z on top).
    // Transparent pixels pass through, allowing clicks on items underneath.
    const auto& order = inventory_->render_order();
    for (auto it = order.rbegin(); it != order.rend(); ++it)
    {
        uint32_t item_id = *it;
        const auto* entry = inventory_->get_bag_item(item_id);
        if (!entry)
            continue;
        if (entry->locked)
            continue; // Locked items are not interactable
        if (inventory_->equipment().find_slot_for(item_id).has_value())
            continue; // Equipped items are not shown in inventory

        // Quick bounding box reject first
        auto rect = item_sprite_rect(item_id);
        if (screen_x < rect.x || screen_x >= rect.x + rect.width ||
            screen_y < rect.y || screen_y >= rect.y + rect.height)
            continue;

        // Pixel-level hit test against actual sprite data
        if (sprite_mgr_)
        {
            auto* spr = sprite_mgr_->get_sprite("item-pack", entry->data.sprite_id - 1);
            if (spr)
            {
                uint32_t frame = static_cast<uint32_t>(entry->data.sprite_frame);
                // local coords relative to the item's draw position (before pivot)
                int32_t local_x = screen_x - (bag.x + entry->pos_x);
                int32_t local_y = screen_y - (bag.y + entry->pos_y);
                if (spr->hit_test(local_x, local_y, frame))
                    return item_id;
                continue; // Transparent pixel — fall through to items below
            }
        }

        // No sprite available — bounding box hit is enough
        return item_id;
    }

    return std::nullopt;
}

bool inventory_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (btn == sf::Mouse::Button::Left)
    {
        // Check resize handle
        int32_t rx = bounds_.x + bounds_.width - resize_handle_size;
        int32_t ry = bounds_.y + bounds_.height - resize_handle_size;
        if (x >= rx && y >= ry && x < bounds_.x + bounds_.width && y < bounds_.y + bounds_.height)
        {
            resizing_ = true;
            resize_start_w_ = bounds_.width;
            resize_start_h_ = bounds_.height;
            resize_start_mx_ = x;
            resize_start_my_ = y;
            return true;
        }

        // Check if clicking on an item — begin dragging it within the bag
        auto hit = item_at(x, y);
        if (hit.has_value() && inventory_)
        {
            uint32_t item_id = hit.value();

            // Double-click detection: same item within threshold → equip it
            bool is_double_click = (item_id == last_clicked_item_id_) &&
                                   (click_elapsed_ < double_click_threshold_);
            last_clicked_item_id_ = item_id;
            click_elapsed_ = 0.0f;

            if (is_double_click && on_equip_)
            {
                const auto* dbl_entry = inventory_->get_bag_item(item_id);
                if (dbl_entry && dbl_entry->data.is_equippable())
                {
                    on_equip_(item_id);
                    return true;
                }
            }

            pressed_item_id_ = item_id;
            dragging_item_id_ = item_id;
            spdlog::debug("Inventory: press on item {} at ({}, {})", item_id, x, y);

            // Bring to front immediately on click
            inventory_->bring_to_front(item_id);

            // Save position before drag for restore on drop-to-world
            const auto* entry = inventory_->get_bag_item(item_id);
            if (entry)
            {
                pre_drag_x_ = entry->pos_x;
                pre_drag_y_ = entry->pos_y;

                // Offset = click position relative to item's draw position
                auto bag = bag_area();
                item_drag_off_x_ = x - (bag.x + entry->pos_x);
                item_drag_off_y_ = y - (bag.y + entry->pos_y);
            }

            // Notify ui_system for cross-dialog drop resolution
            if (on_drag_start_)
                on_drag_start_(item_id, x, y, item_drag_off_x_, item_drag_off_y_);
            return true;
        }

    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool inventory_dialog::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (btn == sf::Mouse::Button::Left)
    {
        if (resizing_)
        {
            resizing_ = false;
            return true;
        }
        pressed_item_id_.reset();
    }

    return dialog::handle_mouse_up(x, y, btn);
}

bool inventory_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    if (resizing_)
    {
        int32_t dx = x - resize_start_mx_;
        int32_t dy = y - resize_start_my_;
        int32_t new_w = std::max(min_width, resize_start_w_ + dx);
        int32_t new_h = std::max(min_height, resize_start_h_ + dy);
        bounds_.width = new_w;
        bounds_.height = new_h;
        return true;
    }

    // Move dragged item's position to follow mouse (legacy behavior)
    if (dragging_item_id_ != 0 && inventory_)
    {
        auto bag = bag_area();
        inventory_->set_position(dragging_item_id_,
                                 static_cast<int16_t>(x - bag.x - item_drag_off_x_),
                                 static_cast<int16_t>(y - bag.y - item_drag_off_y_));
        return true;
    }

    return dialog::handle_mouse_move(x, y);
}

void inventory_dialog::restore_drag_position()
{
    if (dragging_item_id_ == 0 || !inventory_)
        return;
    inventory_->set_position(dragging_item_id_, pre_drag_x_, pre_drag_y_);
}

} // namespace hb
