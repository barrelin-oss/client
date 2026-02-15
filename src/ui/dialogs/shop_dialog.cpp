#include "ui/dialogs/shop_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>
#include <algorithm>

namespace hb
{

// ============ shop_dialog ============

shop_dialog::shop_dialog() : dialog(dialog_type::shop)
{
    set_title("Shop");
    set_bounds({static_cast<int32_t>(screen_width) / 2 - 175, static_cast<int32_t>(screen_height) / 2 - 180, 350, 360});
    set_draggable(true);
    set_closeable(true);
    set_modal(true);
    set_visible(false);
}

void shop_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

void shop_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Shop name
    rend.draw_text(shop_name_, x, y, sf::Color::Yellow);
    y += 20;

    // Player gold
    rend.draw_text(std::format("Your Gold: {}", player_gold_), x, y, sf::Color(255, 220, 100));
    y += 20;

    // Separator
    rend.draw_line(x, y, x + 330, y, sf::Color(80, 80, 100));
    y += 8;

    // Column headers
    rend.draw_text("Item", x, y, sf::Color(150, 150, 180), 12);
    rend.draw_text("Price", x + 180, y, sf::Color(150, 150, 180), 12);
    rend.draw_text("Stock", x + 260, y, sf::Color(150, 150, 180), 12);
    y += 18;

    rend.draw_line(x, y, x + 330, y, sf::Color(60, 60, 80));
    y += 4;

    content_start_y_ = y;

    // Render items for current page
    size_t start_idx = static_cast<size_t>(current_page_ * items_per_page);
    size_t end_idx = std::min(start_idx + items_per_page, items_.size());

    for (size_t i = start_idx; i < end_idx; ++i)
    {
        bool selected = selected_index_.has_value() && selected_index_.value() == i;
        bool hovered = hovered_index_.has_value() && hovered_index_.value() == i;
        render_item_row(rend, items_[i], y, selected, hovered);
        y += item_row_height;
    }

    // Page navigation
    y = bounds_.y + bounds_.height - 80;
    rend.draw_line(x, y, x + 330, y, sf::Color(60, 60, 80));
    y += 8;

    int32_t pages = total_pages();
    if (pages > 1)
    {
        // Prev button
        sf::Color prev_color = current_page_ > 0 ? sf::Color::White : sf::Color(80, 80, 80);
        rend.draw_rect(x, y, 60, 22, sf::Color(50, 50, 70), true);
        rend.draw_text("< Prev", x + 8, y + 4, prev_color, 12);

        // Page indicator
        std::string page_text = std::format("Page {}/{}", current_page_ + 1, pages);
        int32_t page_text_x = x + 165 - static_cast<int32_t>(page_text.length() * 3);
        rend.draw_text(page_text, page_text_x, y + 4, sf::Color::White, 12);

        // Next button
        sf::Color next_color = current_page_ < pages - 1 ? sf::Color::White : sf::Color(80, 80, 80);
        rend.draw_rect(x + 270, y, 60, 22, sf::Color(50, 50, 70), true);
        rend.draw_text("Next >", x + 278, y + 4, next_color, 12);
    }
    y += 30;

    // Buy controls
    if (selected_index_.has_value())
    {
        const auto& item = items_[selected_index_.value()];

        // Quantity selector
        rend.draw_text("Qty:", x, y, sf::Color::White);

        rend.draw_rect(x + 35, y - 2, 20, 20, sf::Color(50, 50, 70), true);
        rend.draw_text("-", x + 41, y, sf::Color::White);

        std::string qty_str = std::to_string(buy_quantity_);
        rend.draw_text(qty_str, x + 62, y, sf::Color::Yellow);

        rend.draw_rect(x + 80, y - 2, 20, 20, sf::Color(50, 50, 70), true);
        rend.draw_text("+", x + 85, y, sf::Color::White);

        // Total cost
        uint32_t total_cost = item.price * buy_quantity_;
        sf::Color cost_color = total_cost <= player_gold_ ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100);
        rend.draw_text(std::format("Total: {}", total_cost), x + 120, y, cost_color);

        // Buy button
        bool can_buy = total_cost <= player_gold_ && (item.stock == 0 || buy_quantity_ <= item.stock);
        sf::Color btn_color = can_buy ? sf::Color(60, 100, 60) : sf::Color(60, 60, 60);
        rend.draw_rect(x + 250, y - 2, 80, 24, btn_color, true);
        rend.draw_text("Buy", x + 278, y + 2, can_buy ? sf::Color::White : sf::Color(100, 100, 100));
    }
}

void shop_dialog::render_item_row(renderer& rend, const shop_item& item, int32_t y, bool selected, bool hovered)
{
    int32_t x = bounds_.x + 10;

    // Row highlight
    if (selected)
    {
        rend.draw_rect(x - 2, y - 2, 334, item_row_height - 2, sf::Color(60, 80, 100), true);
    }
    else if (hovered)
    {
        rend.draw_rect(x - 2, y - 2, 334, item_row_height - 2, sf::Color(50, 50, 70), true);
    }

    // Item icon (colored rect for now)
    sf::Color item_color;
    if (item.item_data.is_weapon())
    {
        item_color = sf::Color(200, 100, 100);
    }
    else if (item.item_data.is_armor())
    {
        item_color = sf::Color(100, 100, 200);
    }
    else if (item.item_data.type == item_type::consume || item.item_data.type == item_type::eat)
    {
        item_color = sf::Color(100, 200, 100);
    }
    else
    {
        item_color = sf::Color(150, 150, 150);
    }
    rend.draw_rect(x, y, 24, 24, item_color, true);

    // Item name
    sf::Color name_color = player_gold_ >= item.price ? sf::Color::White : sf::Color(180, 100, 100);
    rend.draw_text(item.item_data.name, x + 30, y + 4, name_color, 12);

    // Price
    sf::Color price_color = player_gold_ >= item.price ? sf::Color(255, 220, 100) : sf::Color(180, 100, 100);
    rend.draw_text(std::to_string(item.price), x + 180, y + 4, price_color, 12);

    // Stock
    std::string stock_text = item.stock == 0 ? "Inf" : std::to_string(item.stock);
    rend.draw_text(stock_text, x + 260, y + 4, sf::Color(150, 150, 180), 12);
}

std::optional<size_t> shop_dialog::item_index_at(int32_t x, int32_t y) const
{
    if (x < bounds_.x + 8 || x > bounds_.x + bounds_.width - 8)
    {
        return std::nullopt;
    }

    if (y < content_start_y_)
    {
        return std::nullopt;
    }

    int32_t row = (y - content_start_y_) / item_row_height;
    if (row < 0 || row >= items_per_page)
    {
        return std::nullopt;
    }

    size_t idx = static_cast<size_t>(current_page_ * items_per_page + row);
    if (idx >= items_.size())
    {
        return std::nullopt;
    }

    return idx;
}

bool shop_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    int32_t base_x = bounds_.x + 10;

    // Check page buttons
    int32_t page_y = bounds_.y + bounds_.height - 72;
    ui_rect prev_btn{base_x, page_y, 60, 22};
    ui_rect next_btn{base_x + 270, page_y, 60, 22};

    if (prev_btn.contains(x, y))
    {
        prev_page();
        return true;
    }
    if (next_btn.contains(x, y))
    {
        next_page();
        return true;
    }

    // Check quantity buttons
    if (selected_index_.has_value())
    {
        int32_t qty_y = bounds_.y + bounds_.height - 40;
        ui_rect minus_btn{base_x + 35, qty_y - 2, 20, 20};
        ui_rect plus_btn{base_x + 80, qty_y - 2, 20, 20};
        ui_rect buy_btn{base_x + 250, qty_y - 2, 80, 24};

        if (minus_btn.contains(x, y))
        {
            if (buy_quantity_ > 1)
                buy_quantity_--;
            return true;
        }
        if (plus_btn.contains(x, y))
        {
            const auto& item = items_[selected_index_.value()];
            if (item.stock == 0 || buy_quantity_ < item.stock)
            {
                buy_quantity_++;
            }
            return true;
        }
        if (buy_btn.contains(x, y))
        {
            const auto& item = items_[selected_index_.value()];
            uint32_t total_cost = item.price * buy_quantity_;
            if (total_cost <= player_gold_ && (item.stock == 0 || buy_quantity_ <= item.stock))
            {
                if (on_buy_)
                {
                    on_buy_(selected_index_.value(), buy_quantity_);
                }
            }
            return true;
        }
    }

    // Check item selection
    if (btn == sf::Mouse::Button::Left)
    {
        auto idx = item_index_at(x, y);
        if (idx.has_value())
        {
            selected_index_ = idx;
            buy_quantity_ = 1;
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool shop_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    hovered_index_ = item_index_at(x, y);
    return dialog::handle_mouse_move(x, y);
}

void shop_dialog::set_items(const std::vector<shop_item>& items)
{
    items_ = items;
    current_page_ = 0;
    selected_index_.reset();
    buy_quantity_ = 1;
}

void shop_dialog::add_item(const shop_item& item)
{
    items_.push_back(item);
}

void shop_dialog::clear_items()
{
    items_.clear();
    current_page_ = 0;
    selected_index_.reset();
}

void shop_dialog::next_page()
{
    if (current_page_ < total_pages() - 1)
    {
        current_page_++;
        selected_index_.reset();
    }
}

void shop_dialog::prev_page()
{
    if (current_page_ > 0)
    {
        current_page_--;
        selected_index_.reset();
    }
}

int32_t shop_dialog::total_pages() const
{
    return static_cast<int32_t>((items_.size() + items_per_page - 1) / items_per_page);
}

// ============ shop_sell_dialog ============

shop_sell_dialog::shop_sell_dialog() : dialog(dialog_type::shop_sell)
{
    set_title("Sell Items");
    set_bounds({static_cast<int32_t>(screen_width) / 2 - 150, static_cast<int32_t>(screen_height) / 2 - 150, 300, 300});
    set_draggable(true);
    set_closeable(true);
    set_modal(true);
    set_visible(false);
}

void shop_sell_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

void shop_sell_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Header
    rend.draw_text("Select item to sell:", x, y, sf::Color::White);
    y += 24;

    // Separator
    rend.draw_line(x, y, x + 280, y, sf::Color(80, 80, 100));
    y += 8;

    // Column headers
    rend.draw_text("Item", x, y, sf::Color(150, 150, 180), 12);
    rend.draw_text("Value", x + 180, y, sf::Color(150, 150, 180), 12);
    y += 18;

    content_start_y_ = y;

    // Render items
    size_t start_idx = static_cast<size_t>(current_page_ * items_per_page);
    size_t end_idx = std::min(start_idx + items_per_page, items_.size());

    for (size_t i = start_idx; i < end_idx; ++i)
    {
        bool selected = selected_index_.has_value() && selected_index_.value() == i;
        bool hovered = hovered_index_.has_value() && hovered_index_.value() == i;
        render_item_row(rend, items_[i], y, selected, hovered);
        y += item_row_height;
    }

    // Sell button
    if (selected_index_.has_value())
    {
        y = bounds_.y + bounds_.height - 40;
        const auto& item = items_[selected_index_.value()];

        rend.draw_text(std::format("Sell for: {}", item.sell_price), x, y, sf::Color(255, 220, 100));

        rend.draw_rect(x + 200, y - 2, 80, 24, sf::Color(60, 100, 60), true);
        rend.draw_text("Sell", x + 228, y + 2, sf::Color::White);
    }
}

void shop_sell_dialog::render_item_row(renderer& rend, const sell_item& item, int32_t y, bool selected, bool hovered)
{
    int32_t x = bounds_.x + 10;

    // Row highlight
    if (selected)
    {
        rend.draw_rect(x - 2, y - 2, 284, item_row_height - 2, sf::Color(60, 80, 100), true);
    }
    else if (hovered)
    {
        rend.draw_rect(x - 2, y - 2, 284, item_row_height - 2, sf::Color(50, 50, 70), true);
    }

    if (!item.item_ptr)
        return;

    // Item icon
    sf::Color item_color;
    if (item.item_ptr->is_weapon())
    {
        item_color = sf::Color(200, 100, 100);
    }
    else if (item.item_ptr->is_armor())
    {
        item_color = sf::Color(100, 100, 200);
    }
    else if (item.item_ptr->type == item_type::consume || item.item_ptr->type == item_type::eat)
    {
        item_color = sf::Color(100, 200, 100);
    }
    else
    {
        item_color = sf::Color(150, 150, 150);
    }
    rend.draw_rect(x, y, 24, 24, item_color, true);

    // Item name
    rend.draw_text(item.item_ptr->name, x + 30, y + 4, sf::Color::White, 12);

    // Sell price
    rend.draw_text(std::to_string(item.sell_price), x + 180, y + 4, sf::Color(255, 220, 100), 12);
}

std::optional<size_t> shop_sell_dialog::item_index_at(int32_t x, int32_t y) const
{
    if (x < bounds_.x + 8 || x > bounds_.x + bounds_.width - 8)
    {
        return std::nullopt;
    }

    if (y < content_start_y_)
    {
        return std::nullopt;
    }

    int32_t row = (y - content_start_y_) / item_row_height;
    if (row < 0 || row >= items_per_page)
    {
        return std::nullopt;
    }

    size_t idx = static_cast<size_t>(current_page_ * items_per_page + row);
    if (idx >= items_.size())
    {
        return std::nullopt;
    }

    return idx;
}

bool shop_sell_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    // Check sell button
    if (selected_index_.has_value())
    {
        int32_t sell_y = bounds_.y + bounds_.height - 40;
        ui_rect sell_btn{bounds_.x + 210, sell_y - 2, 80, 24};

        if (sell_btn.contains(x, y))
        {
            const auto& item = items_[selected_index_.value()];
            if (on_sell_)
            {
                on_sell_(item.inventory_slot, sell_quantity_);
            }
            return true;
        }
    }

    // Check item selection
    if (btn == sf::Mouse::Button::Left)
    {
        auto idx = item_index_at(x, y);
        if (idx.has_value())
        {
            selected_index_ = idx;
            sell_quantity_ = 1;
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool shop_sell_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    hovered_index_ = item_index_at(x, y);
    return dialog::handle_mouse_move(x, y);
}

void shop_sell_dialog::set_items(const std::vector<sell_item>& items)
{
    items_ = items;
    current_page_ = 0;
    selected_index_.reset();
    sell_quantity_ = 1;
}

void shop_sell_dialog::clear_items()
{
    items_.clear();
    current_page_ = 0;
    selected_index_.reset();
}

} // namespace hb
