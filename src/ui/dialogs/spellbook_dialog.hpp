#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/magic.hpp"
#include <functional>
#include <vector>
#include <optional>

namespace hb {

enum class spellbook_view_mode : uint8_t
{
    classic,  // Organized by level (id / 10 + 1)
    type      // Organized by spell_category
};

// Spellbook dialog - displays known spells organized by level or type
class spellbook_dialog : public dialog {
public:
    static constexpr int32_t row_height = 22;
    static constexpr int32_t icon_size = 14;

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

    // View mode
    void set_view_mode(spellbook_view_mode mode);
    spellbook_view_mode view_mode() const { return view_mode_; }
    void toggle_view_mode();

    // Current level tab (classic mode)
    void set_current_level(uint8_t level);
    uint8_t current_level() const { return current_level_; }

    // Current category tab (type mode)
    void set_current_category(spell_category cat);
    spell_category current_category() const { return current_category_; }

    // Callbacks
    using spell_callback = std::function<void(uint16_t)>;
    void set_on_spell_click(spell_callback callback) { on_spell_click_ = std::move(callback); }

private:
    void render_spell_row(renderer& rend, const spell& sp, int32_t x, int32_t y,
                          int32_t width, bool selected, bool hovered);
    std::optional<size_t> spell_index_at(int32_t x, int32_t y) const;

    void rebuild_filtered_spells();
    void render_classic_tabs(renderer& rend, int32_t x, int32_t& y);
    void render_type_tabs(renderer& rend, int32_t x, int32_t& y);

    sf::Color spell_type_color(magic_type type) const;

    // Compute display level from spell ID: id / 10 + 1
    static uint8_t spell_level(uint16_t id) { return static_cast<uint8_t>(id / 10 + 1); }
    static const char* category_label(spell_category cat);

    std::vector<spell> spells_;
    std::vector<spell> filtered_spells_;
    bool filter_dirty_ = true;

    // View mode
    spellbook_view_mode view_mode_ = spellbook_view_mode::classic;

    // Classic mode state
    uint8_t current_level_ = 1;  // 1-10
    static constexpr uint8_t max_levels = 10;

    // Type mode state
    spell_category current_category_ = spell_category::attack;

    std::optional<uint16_t> selected_spell_;
    std::optional<size_t> hovered_index_;

    spell_callback on_spell_click_;

    int32_t content_start_y_ = 0;
    int32_t content_width_ = 0;
    int32_t tabs_y_ = 0;  // Y position of tabs row for hit-testing
};

} // namespace hb
