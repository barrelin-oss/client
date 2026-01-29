#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/magic.hpp"
#include <functional>
#include <vector>
#include <optional>

namespace hb {

// Spellbook dialog - displays known spells organized by circle
class spellbook_dialog : public dialog {
public:
    static constexpr int32_t spells_per_row = 5;
    static constexpr int32_t spell_slot_size = 40;
    static constexpr int32_t slot_padding = 4;

    spellbook_dialog();
    ~spellbook_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set known spells
    void set_spells(const std::vector<spell>& spells);
    void add_spell(const spell& sp);
    void clear_spells();

    // Set selected spell (hotkey assignment)
    void set_selected_spell(uint16_t id) { selected_spell_ = id; }
    std::optional<uint16_t> selected_spell() const { return selected_spell_; }

    // Current circle (page)
    void set_current_circle(uint8_t circle);
    uint8_t current_circle() const { return current_circle_; }

    // Callbacks
    using spell_callback = std::function<void(uint16_t)>;
    void set_on_spell_click(spell_callback callback) { on_spell_click_ = std::move(callback); }
    void set_on_spell_double_click(spell_callback callback) { on_spell_double_click_ = std::move(callback); }

private:
    void render_spell_slot(renderer& rend, const spell& sp, int32_t x, int32_t y, bool selected);
    std::optional<size_t> spell_index_at(int32_t x, int32_t y) const;

    std::vector<spell> spells_;
    std::vector<spell> current_circle_spells_;

    uint8_t current_circle_ = 1;  // 1-10
    static constexpr uint8_t max_circles = 10;

    std::optional<uint16_t> selected_spell_;
    std::optional<size_t> hovered_index_;

    spell_callback on_spell_click_;
    spell_callback on_spell_double_click_;

    // Double-click tracking
    size_t last_click_index_ = SIZE_MAX;
    float click_timer_ = 0.0f;
    static constexpr float double_click_time = 0.3f;

    int32_t content_start_y_ = 0;
};

} // namespace hb
