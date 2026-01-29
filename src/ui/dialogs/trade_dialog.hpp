#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/item.hpp"
#include <functional>
#include <vector>
#include <string>
#include <array>
#include <optional>

namespace hb {

// Trade dialog - for player-to-player trading
class trade_dialog : public dialog {
public:
    static constexpr int32_t trade_slots = 8;  // 8 slots per player
    static constexpr int32_t slot_size = 36;
    static constexpr int32_t slot_padding = 2;

    trade_dialog();
    ~trade_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Partner info
    void set_partner_name(std::string_view name) { partner_name_ = name; }
    std::string_view partner_name() const { return partner_name_; }

    // My offered items
    void set_my_item(int32_t slot, const item* itm);
    void clear_my_slot(int32_t slot);
    void clear_my_items();

    // Partner's offered items
    void set_partner_item(int32_t slot, const item* itm);
    void clear_partner_slot(int32_t slot);
    void clear_partner_items();

    // Gold
    void set_my_gold(uint32_t gold) { my_gold_ = gold; }
    void set_partner_gold(uint32_t gold) { partner_gold_ = gold; }

    // Confirmation state
    void set_my_confirmed(bool confirmed) { my_confirmed_ = confirmed; }
    void set_partner_confirmed(bool confirmed) { partner_confirmed_ = confirmed; }
    bool my_confirmed() const { return my_confirmed_; }
    bool partner_confirmed() const { return partner_confirmed_; }

    // Callbacks
    using slot_callback = std::function<void(int32_t slot)>;
    using confirm_callback = std::function<void()>;
    using cancel_callback = std::function<void()>;
    using gold_callback = std::function<void(uint32_t amount)>;

    void set_on_add_item(slot_callback callback) { on_add_item_ = std::move(callback); }
    void set_on_remove_item(slot_callback callback) { on_remove_item_ = std::move(callback); }
    void set_on_confirm(confirm_callback callback) { on_confirm_ = std::move(callback); }
    void set_on_cancel(cancel_callback callback) { on_cancel_ = std::move(callback); }
    void set_on_set_gold(gold_callback callback) { on_set_gold_ = std::move(callback); }

private:
    void render_slot(renderer& rend, int32_t x, int32_t y, const item* itm, bool hovered);
    std::optional<int32_t> slot_at(int32_t x, int32_t y) const;

    std::string partner_name_;

    std::array<const item*, trade_slots> my_items_{};
    std::array<const item*, trade_slots> partner_items_{};

    uint32_t my_gold_ = 0;
    uint32_t partner_gold_ = 0;

    bool my_confirmed_ = false;
    bool partner_confirmed_ = false;

    std::optional<int32_t> hovered_slot_;
    bool hovering_my_side_ = true;

    slot_callback on_add_item_;
    slot_callback on_remove_item_;
    confirm_callback on_confirm_;
    cancel_callback on_cancel_;
    gold_callback on_set_gold_;
};

} // namespace hb
