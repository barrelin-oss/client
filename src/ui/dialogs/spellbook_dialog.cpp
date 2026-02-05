#include "ui/dialogs/spellbook_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <format>

namespace hb {

// Category tabs displayed in type mode
static constexpr spell_category category_tabs[] = {
    spell_category::attack,
    spell_category::healing,
    spell_category::buff,
    spell_category::debuff,
    spell_category::utility,
};
static constexpr size_t num_category_tabs = std::size(category_tabs);

spellbook_dialog::spellbook_dialog()
    : dialog(dialog_type::spellbook)
{
    set_title("Spellbook");
    set_bounds({100, 80, 280, 380});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
    set_drag_clamp(drag_clamp::partial);
}

void spellbook_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

void spellbook_dialog::render(renderer& rend)
{
    if (!visible_) return;

    if (filter_dirty_)
    {
        rebuild_filtered_spells();
    }

    dialog::render(rend);

    int32_t x = bounds_.x + 8;
    int32_t y = bounds_.y + 28;
    content_width_ = bounds_.width - 16;

    // View mode toggle: [Classic] [Type]
    {
        bool classic_active = (view_mode_ == spellbook_view_mode::classic);

        sf::Color classic_bg = classic_active ? sf::Color(80, 80, 120) : sf::Color(40, 40, 55);
        sf::Color type_bg = classic_active ? sf::Color(40, 40, 55) : sf::Color(80, 80, 120);
        sf::Color classic_text = classic_active ? sf::Color::Yellow : sf::Color(160, 160, 160);
        sf::Color type_text = classic_active ? sf::Color(160, 160, 160) : sf::Color::Yellow;

        rend.draw_rect(x, y, 60, 16, classic_bg, true);
        rend.draw_rect(x, y, 60, 16, sf::Color(80, 80, 100), false);
        rend.draw_text("Classic", x + 6, y + 2, classic_text, 11);

        rend.draw_rect(x + 64, y, 50, 16, type_bg, true);
        rend.draw_rect(x + 64, y, 50, 16, sf::Color(80, 80, 100), false);
        rend.draw_text("Type", x + 64 + 10, y + 2, type_text, 11);
    }
    y += 22;

    // Draw tabs based on view mode
    tabs_y_ = y;
    if (view_mode_ == spellbook_view_mode::classic)
    {
        render_classic_tabs(rend, x, y);
    }
    else
    {
        render_type_tabs(rend, x, y);
    }

    // Separator
    rend.draw_line(x, y, x + content_width_, y, sf::Color(80, 80, 100));
    y += 4;

    content_start_y_ = y;

    if (filtered_spells_.empty())
    {
        const char* empty_msg = (view_mode_ == spellbook_view_mode::classic)
            ? "No spells at this level"
            : "No spells in this category";
        rend.draw_text(empty_msg, x + 4, y + 8, sf::Color(120, 120, 120), 11);
        return;
    }

    // Draw spell rows
    for (size_t i = 0; i < filtered_spells_.size(); ++i)
    {
        int32_t row_y = y + static_cast<int32_t>(i) * row_height;

        // Stop if we'd draw past dialog bottom
        if (row_y + row_height > bounds_.y + bounds_.height - 4)
            break;

        bool is_selected = selected_spell_.has_value() &&
                          selected_spell_.value() == filtered_spells_[i].id;
        bool is_hovered = hovered_index_.has_value() &&
                         hovered_index_.value() == i;

        render_spell_row(rend, filtered_spells_[i], x, row_y, content_width_,
                         is_selected, is_hovered);
    }
}

void spellbook_dialog::render_classic_tabs(renderer& rend, int32_t x, int32_t& y)
{
    for (uint8_t level = 1; level <= max_levels; ++level)
    {
        int32_t tab_x = x + (level - 1) * 25;
        bool is_current = (level == current_level_);

        bool has_spells = false;
        for (const auto& sp : spells_)
        {
            if (spell_level(sp.id) == level)
            {
                has_spells = true;
                break;
            }
        }

        sf::Color bg = is_current ? sf::Color(80, 80, 120)
                     : has_spells ? sf::Color(50, 50, 70)
                                  : sf::Color(35, 35, 45);
        rend.draw_rect(tab_x, y, 22, 18, bg, true);
        rend.draw_rect(tab_x, y, 22, 18, sf::Color(80, 80, 100), false);

        sf::Color text_color = is_current ? sf::Color::Yellow
                             : has_spells ? sf::Color::White
                                          : sf::Color(80, 80, 80);
        rend.draw_text(std::to_string(level), tab_x + 7, y + 2, text_color, 12);
    }
    y += 24;
}

void spellbook_dialog::render_type_tabs(renderer& rend, int32_t x, int32_t& y)
{
    static constexpr int32_t tab_width = 48;
    static constexpr int32_t tab_gap = 2;

    for (size_t i = 0; i < num_category_tabs; ++i)
    {
        spell_category cat = category_tabs[i];
        int32_t tab_x = x + static_cast<int32_t>(i) * (tab_width + tab_gap);
        bool is_current = (cat == current_category_);

        bool has_spells = false;
        for (const auto& sp : spells_)
        {
            if (sp.category == cat)
            {
                has_spells = true;
                break;
            }
        }

        sf::Color bg = is_current ? sf::Color(80, 80, 120)
                     : has_spells ? sf::Color(50, 50, 70)
                                  : sf::Color(35, 35, 45);
        rend.draw_rect(tab_x, y, tab_width, 18, bg, true);
        rend.draw_rect(tab_x, y, tab_width, 18, sf::Color(80, 80, 100), false);

        sf::Color text_color = is_current ? sf::Color::Yellow
                             : has_spells ? sf::Color::White
                                          : sf::Color(80, 80, 80);
        rend.draw_text(category_label(cat), tab_x + 4, y + 2, text_color, 11);
    }
    y += 24;
}

void spellbook_dialog::rebuild_filtered_spells()
{
    filtered_spells_.clear();

    if (view_mode_ == spellbook_view_mode::classic)
    {
        for (const auto& sp : spells_)
        {
            if (spell_level(sp.id) == current_level_)
            {
                filtered_spells_.push_back(sp);
            }
        }
    }
    else
    {
        for (const auto& sp : spells_)
        {
            if (sp.category == current_category_)
            {
                filtered_spells_.push_back(sp);
            }
        }
    }

    std::sort(filtered_spells_.begin(), filtered_spells_.end(),
        [](const spell& a, const spell& b) { return a.id < b.id; });

    filter_dirty_ = false;
}

void spellbook_dialog::render_spell_row(renderer& rend, const spell& sp,
                                         int32_t x, int32_t y, int32_t width,
                                         bool selected, bool hovered)
{
    bool is_learned = sp.learned;

    // Row background
    sf::Color bg;
    if (!is_learned)
    {
        bg = sf::Color(25, 25, 30, 180);
    }
    else if (selected)
    {
        bg = sf::Color(50, 70, 110);
    }
    else if (hovered)
    {
        bg = sf::Color(50, 50, 70);
    }
    else
    {
        bg = sf::Color(35, 35, 50, 200);
    }
    rend.draw_rect(x, y, width, row_height - 1, bg, true);

    // Type icon
    sf::Color icon_color = spell_type_color(sp.type);
    if (!is_learned)
    {
        icon_color.r /= 3;
        icon_color.g /= 3;
        icon_color.b /= 3;
    }
    int32_t icon_y = y + (row_height - icon_size) / 2;
    rend.draw_rect(x + 4, icon_y, icon_size, icon_size, icon_color, true);
    rend.draw_rect(x + 4, icon_y, icon_size, icon_size,
                   sf::Color(icon_color.r / 2, icon_color.g / 2, icon_color.b / 2), false);

    // Spell name
    sf::Color name_color = is_learned ? sf::Color::White : sf::Color(70, 70, 70);
    rend.draw_text(sp.name, x + 4 + icon_size + 6, y + 3, name_color, 12);

    // Mana cost (right-aligned)
    std::string mp_text = std::to_string(sp.mp_cost);
    // Approximate right-alignment: each char ~7px at size 12
    int32_t mp_width = static_cast<int32_t>(mp_text.length()) * 7;
    sf::Color mp_color = is_learned ? sf::Color(120, 160, 255) : sf::Color(50, 50, 70);
    rend.draw_text(mp_text, x + width - mp_width - 6, y + 3, mp_color, 12);
}

sf::Color spellbook_dialog::spell_type_color(magic_type type) const
{
    switch (type)
    {
        case magic_type::damage_spot:
        case magic_type::damage_area:
        case magic_type::damage_area_no_spot:
        case magic_type::bloody_shock_wave:
        case magic_type::tremor:
        case magic_type::ice:
        case magic_type::earthworm_strike:
        case magic_type::earth_shock_wave:
        case magic_type::mass_magic_missile:
            return sf::Color(200, 80, 80);    // Red for damage

        case magic_type::hp_up_spot:
        case magic_type::sp_up_spot:
        case magic_type::sp_up_area:
        case magic_type::resurrection:
            return sf::Color(80, 200, 80);    // Green for healing

        case magic_type::protect:
        case magic_type::berserk:
        case magic_type::invisibility:
            return sf::Color(80, 140, 220);   // Blue for buffs

        case magic_type::hold_object:
        case magic_type::poison:
        case magic_type::confuse:
        case magic_type::armor_break:
        case magic_type::cancellation:
        case magic_type::inhibition_casting:
            return sf::Color(170, 80, 200);   // Purple for debuffs

        default:
            return sf::Color(140, 140, 170);  // Grey for utility
    }
}

std::optional<size_t> spellbook_dialog::spell_index_at(int32_t x, int32_t y) const
{
    if (content_start_y_ == 0) return std::nullopt;
    if (filtered_spells_.empty()) return std::nullopt;

    int32_t base_x = bounds_.x + 8;

    // Check if x is within the content area
    if (x < base_x || x > base_x + content_width_) return std::nullopt;
    if (y < content_start_y_) return std::nullopt;

    int32_t rel_y = y - content_start_y_;
    auto idx = static_cast<size_t>(rel_y / row_height);

    if (idx < filtered_spells_.size())
    {
        // Check we're not past the dialog bottom
        int32_t row_y = content_start_y_ + static_cast<int32_t>(idx) * row_height;
        if (row_y + row_height <= bounds_.y + bounds_.height - 4)
        {
            return idx;
        }
    }

    return std::nullopt;
}

bool spellbook_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_) return false;
    if (!bounds_.contains(x, y)) return false;

    // Ensure filtered spells are current
    if (filter_dirty_)
    {
        rebuild_filtered_spells();
    }

    int32_t base_x = bounds_.x + 8;

    // Check close button
    if (closeable_)
    {
        ui_rect close_rect{bounds_.x + bounds_.width - 20, bounds_.y + 4, 16, 16};
        if (close_rect.contains(x, y))
        {
            close();
            return true;
        }
    }

    // Check view mode toggle buttons
    int32_t toggle_y = bounds_.y + 28;
    ui_rect classic_btn{base_x, toggle_y, 60, 16};
    ui_rect type_btn{base_x + 64, toggle_y, 50, 16};

    if (classic_btn.contains(x, y))
    {
        set_view_mode(spellbook_view_mode::classic);
        return true;
    }
    if (type_btn.contains(x, y))
    {
        set_view_mode(spellbook_view_mode::type);
        return true;
    }

    // Check tabs
    if (view_mode_ == spellbook_view_mode::classic)
    {
        for (uint8_t level = 1; level <= max_levels; ++level)
        {
            int32_t tab_x = base_x + (level - 1) * 25;
            ui_rect tab_rect{tab_x, tabs_y_, 22, 18};
            if (tab_rect.contains(x, y))
            {
                set_current_level(level);
                return true;
            }
        }
    }
    else
    {
        static constexpr int32_t tab_width = 48;
        static constexpr int32_t tab_gap = 2;
        for (size_t i = 0; i < num_category_tabs; ++i)
        {
            int32_t tab_x = base_x + static_cast<int32_t>(i) * (tab_width + tab_gap);
            ui_rect tab_rect{tab_x, tabs_y_, tab_width, 18};
            if (tab_rect.contains(x, y))
            {
                set_current_category(category_tabs[i]);
                return true;
            }
        }
    }

    // Check spell rows
    if (btn == sf::Mouse::Button::Left)
    {
        auto idx = spell_index_at(x, y);
        if (idx.has_value() && idx.value() < filtered_spells_.size())
        {
            const auto& clicked_spell = filtered_spells_[idx.value()];
            if (clicked_spell.learned)
            {
                uint16_t clicked_id = clicked_spell.id;
                selected_spell_ = clicked_id;

                if (on_spell_click_)
                {
                    spdlog::info("Spellbook: clicked spell '{}' (id={})", clicked_spell.name, clicked_id);
                    on_spell_click_(clicked_id);
                }
            }
            return true;
        }
    }

    // Click was in dialog but not on any interactive element - start drag
    if (btn == sf::Mouse::Button::Left && draggable_)
    {
        dragging_ = true;
        drag_offset_x_ = x - bounds_.x;
        drag_offset_y_ = y - bounds_.y;
    }
    return true;
}

bool spellbook_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_) return false;

    if (filter_dirty_)
    {
        rebuild_filtered_spells();
    }

    hovered_index_ = spell_index_at(x, y);

    return dialog::handle_mouse_move(x, y);
}

void spellbook_dialog::set_spells(const std::vector<spell>& spells)
{
    spells_ = spells;
    filter_dirty_ = true;
}

void spellbook_dialog::add_spell(const spell& sp)
{
    auto it = std::find_if(spells_.begin(), spells_.end(),
        [&sp](const spell& existing) { return existing.id == sp.id; });

    if (it != spells_.end())
    {
        *it = sp;
    }
    else
    {
        spells_.push_back(sp);
    }
    filter_dirty_ = true;
}

void spellbook_dialog::clear_spells()
{
    spells_.clear();
    filtered_spells_.clear();
    selected_spell_.reset();
    hovered_index_.reset();
    filter_dirty_ = true;
}

void spellbook_dialog::set_view_mode(spellbook_view_mode mode)
{
    if (view_mode_ != mode)
    {
        view_mode_ = mode;
        hovered_index_.reset();
        filter_dirty_ = true;
    }
}

void spellbook_dialog::toggle_view_mode()
{
    set_view_mode(view_mode_ == spellbook_view_mode::classic
        ? spellbook_view_mode::type
        : spellbook_view_mode::classic);
}

void spellbook_dialog::set_current_level(uint8_t level)
{
    if (level >= 1 && level <= max_levels)
    {
        current_level_ = level;
        hovered_index_.reset();
        filter_dirty_ = true;
    }
}

void spellbook_dialog::set_current_category(spell_category cat)
{
    current_category_ = cat;
    hovered_index_.reset();
    filter_dirty_ = true;
}

const char* spellbook_dialog::category_label(spell_category cat)
{
    switch (cat)
    {
        case spell_category::attack:  return "Atk";
        case spell_category::defense: return "Def";
        case spell_category::healing: return "Heal";
        case spell_category::buff:    return "Buff";
        case spell_category::debuff:  return "Debuf";
        case spell_category::summon:  return "Summ";
        case spell_category::utility: return "Util";
    }
    return "???";
}

} // namespace hb
