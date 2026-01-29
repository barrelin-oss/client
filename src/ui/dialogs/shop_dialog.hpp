#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace hb {

// Shop item for sale
struct shop_item {
    item item_data;
    uint32_t price = 0;
    uint32_t stock = 0;  // 0 = unlimited
};

// Shop dialog - for buying items from NPCs
class shop_dialog : public dialog {
public:
    static constexpr int32_t items_per_page = 8;
    static constexpr int32_t item_row_height = 32;

    shop_dialog();
    ~shop_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set shop data
    void set_shop_name(std::string_view name) { shop_name_ = name; }
    void set_items(const std::vector<shop_item>& items);
    void add_item(const shop_item& item);
    void clear_items();

    // Player gold (for affordability display)
    void set_player_gold(uint32_t gold) { player_gold_ = gold; }

    // Page navigation
    void next_page();
    void prev_page();
    int32_t current_page() const { return current_page_; }
    int32_t total_pages() const;

    // Callbacks
    using buy_callback = std::function<void(size_t item_index, uint32_t quantity)>;
    void set_on_buy(buy_callback callback) { on_buy_ = std::move(callback); }

    // Selected item
    std::optional<size_t> selected_index() const { return selected_index_; }
    void set_selected_index(std::optional<size_t> idx) { selected_index_ = idx; }

private:
    void render_item_row(renderer& rend, const shop_item& item, int32_t y, bool selected, bool hovered);
    std::optional<size_t> item_index_at(int32_t x, int32_t y) const;

    std::string shop_name_;
    std::vector<shop_item> items_;
    uint32_t player_gold_ = 0;

    int32_t current_page_ = 0;
    std::optional<size_t> selected_index_;
    std::optional<size_t> hovered_index_;

    buy_callback on_buy_;

    // Buy quantity
    uint32_t buy_quantity_ = 1;

    int32_t content_start_y_ = 0;
};

// Shop sell dialog - for selling items to NPCs
class shop_sell_dialog : public dialog {
public:
    static constexpr int32_t items_per_page = 8;
    static constexpr int32_t item_row_height = 32;

    shop_sell_dialog();
    ~shop_sell_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set sellable items (from inventory)
    struct sell_item {
        const item* item_ptr = nullptr;
        int32_t inventory_slot = -1;
        uint32_t sell_price = 0;
    };

    void set_items(const std::vector<sell_item>& items);
    void clear_items();

    // Callbacks
    using sell_callback = std::function<void(int32_t inventory_slot, uint32_t quantity)>;
    void set_on_sell(sell_callback callback) { on_sell_ = std::move(callback); }

private:
    void render_item_row(renderer& rend, const sell_item& item, int32_t y, bool selected, bool hovered);
    std::optional<size_t> item_index_at(int32_t x, int32_t y) const;

    std::vector<sell_item> items_;

    int32_t current_page_ = 0;
    std::optional<size_t> selected_index_;
    std::optional<size_t> hovered_index_;

    sell_callback on_sell_;
    uint32_t sell_quantity_ = 1;

    int32_t content_start_y_ = 0;
};

} // namespace hb
