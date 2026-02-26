#include "ui/dialogs/character_dialog.hpp"
#include "entity/entity.hpp"
#include "entity/components.hpp"
#include "gameplay/inventory.hpp"
#include "graphics/renderer.hpp"
#include "graphics/paperdoll_renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "input/input.hpp"
#include <algorithm>
#include <format>

namespace hb
{

character_dialog::character_dialog() : dialog(dialog_type::character_info)
{
    set_title("Character Info");
    set_bounds({20, 50, dialog_width, dialog_height});
    set_draggable(true);
    set_drag_anywhere(true);
    set_closeable(true);
    set_visible(false);
    set_drag_clamp(drag_clamp::partial);
}

void character_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
    click_elapsed_ += delta_time;
}

void character_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t y = bounds_.y + title_bar_height + 6;

    render_name_section(rend, y);

    // Separator
    rend.draw_line(bounds_.x + padding, y, bounds_.x + dialog_width - padding, y, sf::Color(80, 80, 100));
    y += 4;

    render_equipment_and_stats(rend, y);

    // Separator
    rend.draw_line(bounds_.x + padding, y, bounds_.x + dialog_width - padding, y, sf::Color(80, 80, 100));
    y += 6;

    render_attributes(rend, y);

    // Separator
    rend.draw_line(bounds_.x + padding, y, bounds_.x + dialog_width - padding, y, sf::Color(80, 80, 100));
    y += 6;

    render_buttons(rend, y);
}

void character_dialog::render_name_section(renderer& rend, int32_t& y)
{
    int32_t cx = bounds_.x + dialog_width / 2;

    // Section header
    const char* header = "Name and Status";
    int32_t header_w = static_cast<int32_t>(15) * 6; // strlen("Name and Status") = 15
    rend.draw_text(header, cx - header_w / 2, y, sf::Color(180, 180, 200));
    y += 18;

    // Name (title will go here later)
    std::string name_str = "Unknown";
    std::string faction_str = "Traveler";

    if (player_ && player_->has_name())
        name_str = player_->name().name;

    int32_t name_w = static_cast<int32_t>(rend.text().measure_width(name_str, 14));
    rend.draw_text(name_str, cx - name_w / 2, y, sf::Color::White);
    y += 18;

    // Faction line
    if (player_ && player_->has_name())
    {
        const auto& nc = player_->name();
        if (nc.faction == "aresden")
            faction_str = "Aresden";
        else if (nc.faction == "elvine")
            faction_str = "Elvine";
        else
            faction_str = "Traveler";

        if (!nc.guild_name.empty())
        {
            faction_str += std::format(" ({} {})",
                                       nc.guild_name,
                                       nc.guild_rank_id == 0 ? "GuildMaster" : "Guildsman");
        }
    }

    int32_t faction_w = static_cast<int32_t>(faction_str.length()) * 6;
    rend.draw_text(faction_str, cx - faction_w / 2, y, sf::Color(200, 200, 150));
    y += 22;
}

void character_dialog::render_equipment_and_stats(renderer& rend, int32_t& y)
{
    int32_t cx = bounds_.x + dialog_width / 2;

    // Section header — centered
    const char* header = "Equipment and Characteristics";
    int32_t header_w = static_cast<int32_t>(29) * 6; // strlen = 29
    rend.draw_text(header, cx - header_w / 2, y, sf::Color(180, 180, 200));
    y += 20;

    // Left side: paperdoll
    // Legacy area is ~100x170 with the sprite anchor at (sX+171, sY+290).
    // The area top-left is at roughly (sX+15, sY+88), so the anchor is
    // 156px right and 202px down from the area origin. Sprites use built-in
    // pivot offsets to render back into the visible box.
    int32_t paperdoll_x = bounds_.x + padding;
    int32_t paperdoll_y = y;
    // Draw paperdoll background
    rend.draw_rect(paperdoll_x, paperdoll_y, paperdoll_area_w, paperdoll_area_h, sf::Color(30, 30, 45), true);
    rend.draw_rect(paperdoll_x, paperdoll_y, paperdoll_area_w, paperdoll_area_h, sf::Color(60, 60, 80), false);

    // Anchor position: legacy offset from paperdoll area top-left
    int32_t anchor_x = paperdoll_x + 176;
    int32_t anchor_y = paperdoll_y + 187;
    paperdoll_anchor_x_ = anchor_x;
    paperdoll_anchor_y_ = anchor_y;

    // Equipment is rendered directly onto the character sprite — no icon grid
    render_paperdoll(rend, anchor_x, anchor_y);

    // Right side: stats text (matching legacy labels)
    int32_t stat_x = paperdoll_x + paperdoll_area_w + 16;
    int32_t stat_y = y;

    uint16_t level = 0;
    int64_t exp = 0;
    int64_t exp_next = 0;
    int32_t hp = 0, max_hp = 0;
    int32_t mp = 0, max_mp = 0;
    int32_t sp = 0, max_sp = 0;
    int32_t ek = 0, pk = 0;

    if (player_ && player_->has_stats())
    {
        const auto& s = player_->stats();
        level = s.level;
        exp = s.experience;
        exp_next = s.experience_to_next;
        hp = s.hp;
        max_hp = s.max_hp;
        mp = s.mp;
        max_mp = s.max_mp;
        sp = s.sp;
        max_sp = s.max_sp;
    }
    if (player_ && player_->has_combat())
    {
        const auto& c = player_->combat();
        ek = c.enemy_kill_count;
        pk = c.pk_count;
    }

    rend.draw_text(std::format("Level: {}", level), stat_x, stat_y, sf::Color::White);
    stat_y += 16;
    rend.draw_text(std::format("EXP: {}", exp), stat_x, stat_y, sf::Color::White);
    stat_y += 16;
    rend.draw_text(std::format("Next EXP: {}", exp_next), stat_x, stat_y, sf::Color::White);
    stat_y += 20;

    // Separator
    rend.draw_line(stat_x, stat_y, bounds_.x + dialog_width - padding, stat_y, sf::Color(60, 60, 80));
    stat_y += 6;

    // Health/Mana/Stamina
    rend.draw_text(std::format("Health: {} / {}", hp, max_hp), stat_x, stat_y, sf::Color(220, 80, 80));
    stat_y += 16;
    rend.draw_text(std::format("Mana: {} / {}", mp, max_mp), stat_x, stat_y, sf::Color(80, 80, 220));
    stat_y += 16;
    rend.draw_text(std::format("Stamina: {} / {}", sp, max_sp), stat_x, stat_y, sf::Color(80, 220, 80));
    stat_y += 20;

    // Separator
    rend.draw_line(stat_x, stat_y, bounds_.x + dialog_width - padding, stat_y, sf::Color(60, 60, 80));
    stat_y += 6;

    // Max Load / EK Count / PK Count
    uint32_t weight = inventory_ ? inventory_->current_weight() : 0;
    uint32_t max_weight = inventory_ ? inventory_->max_weight() : 0;

    rend.draw_text(std::format("Weight: {} / {}", weight, max_weight), stat_x, stat_y, sf::Color::White);
    stat_y += 16;
    rend.draw_text(std::format("EK Count: {}", ek), stat_x, stat_y, sf::Color::White);
    stat_y += 16;
    rend.draw_text(std::format("PK Count: {}", pk), stat_x, stat_y, sf::Color::White);
    stat_y += 10;

    // y advances to whichever column is taller
    y = std::max(paperdoll_y + paperdoll_area_h, stat_y) + 6;
}

void character_dialog::render_paperdoll(renderer& rend, int32_t x, int32_t y)
{
    if (!paperdoll_ || !sprite_mgr_ || !player_)
        return;

    const auto& spr = player_->sprite();
    paperdoll_->draw(rend, *sprite_mgr_, x, y, spr.gender, spr.skin_color, spr.hair_style, spr.hair_color,
                     spr.underwear_color, inventory_, dragging_slot_);
}

void character_dialog::render_attributes(renderer& rend, int32_t& y)
{
    int32_t x = bounds_.x + padding + 10;
    int32_t col_width = (dialog_width - padding * 2 - 10) / 2;

    uint16_t str = 0, int_val = 0, vit = 0, dex = 0, mag = 0, cha = 0;

    if (player_ && player_->has_stats())
    {
        const auto& s = player_->stats();
        str = s.strength;
        int_val = s.intelligence;
        vit = s.vitality;
        dex = s.dexterity;
        mag = s.magic;
        cha = s.charisma;
    }

    // Stat points header
    if (stat_points_ > 0)
    {
        rend.draw_text(std::format("Points: {}", stat_points_), x, y, sf::Color(100, 255, 100));
        y += 18;
    }

    // Save the y position for hit testing
    attr_y_ = y;

    // 2 columns x 3 rows — pixel-accurate alignment using text measurement
    static constexpr uint32_t font_size = 14;
    static constexpr int32_t gap = 6; // pixels between label and value fields

    // Measure divider positions from longest label in each column
    auto& text = rend.text();
    auto measure = [&](std::string_view s) { return static_cast<int32_t>(text.measure_width(s, font_size)); };

    int32_t left_divider = std::max({measure("Strength"), measure("Vitality"), measure("Magic")});
    int32_t right_divider = std::max({measure("Intelligence"), measure("Dexterity"), measure("Charisma")});
    int32_t value_field = measure("200"); // stats range 0-200

    auto draw_attr = [&](const char* label, uint16_t value, int32_t col, int32_t row,
                         [[maybe_unused]] int32_t stat_index)
    {
        int32_t ax = x + col * col_width;
        int32_t ay = y + row * 20;
        int32_t divider = (col == 0) ? left_divider : right_divider;

        // Right-align label to divider
        int32_t label_w = measure(label);
        int32_t label_x = ax + divider - label_w;
        rend.draw_text(label, label_x, ay, sf::Color(200, 200, 255));

        // Right-align value after gap
        auto val_str = std::format("{}", value);
        int32_t val_w = measure(val_str);
        int32_t val_x = ax + divider + gap + value_field - val_w;
        rend.draw_text(val_str, val_x, ay, sf::Color(200, 200, 255));

        if (stat_points_ > 0)
        {
            int32_t btn_x = ax + col_width - stat_button_size - 4;
            int32_t btn_y = ay - 2;
            rend.draw_rect(btn_x, btn_y, stat_button_size, stat_button_size, sf::Color(60, 100, 60), true);
            rend.draw_rect(btn_x, btn_y, stat_button_size, stat_button_size, sf::Color(80, 140, 80), false);
            rend.draw_text("+", btn_x + 4, btn_y, sf::Color::White);
        }
    };

    draw_attr("Strength", str, 0, 0, 0);
    draw_attr("Intelligence", int_val, 1, 0, 3);
    draw_attr("Vitality", vit, 0, 1, 1);
    draw_attr("Dexterity", dex, 1, 1, 2);
    draw_attr("Magic", mag, 0, 2, 4);
    draw_attr("Charisma", cha, 1, 2, 5);

    y += 3 * 20 + 6;
}

void character_dialog::render_buttons(renderer& rend, int32_t y)
{
    buttons_y_ = y;
    int32_t total_btn_width = 3 * button_width + 2 * 8;
    int32_t btn_start_x = bounds_.x + (dialog_width - total_btn_width) / 2;

    const char* labels[] = {"Quest", "Party", "Level Set."};

    for (int i = 0; i < 3; ++i)
    {
        int32_t bx = btn_start_x + i * (button_width + 8);

        sf::Color bg = (hovered_button_ == i) ? sf::Color(60, 60, 80) : sf::Color(40, 40, 55);
        sf::Color border = (hovered_button_ == i) ? sf::Color(100, 100, 140) : sf::Color(70, 70, 90);

        rend.draw_rect(bx, y, button_width, button_height, bg, true);
        rend.draw_rect(bx, y, button_width, button_height, border, false);

        int32_t text_len = 0;
        for (const char* p = labels[i]; *p; ++p)
            ++text_len;
        int32_t text_width = text_len * 6;
        rend.draw_text(labels[i], bx + (button_width - text_width) / 2, y + 5, sf::Color::White);
    }

    // Update dialog height to fit content
    int32_t content_bottom = y + button_height + padding;
    int32_t needed_height = content_bottom - bounds_.y;
    if (needed_height != bounds_.height)
    {
        bounds_.height = needed_height;
    }
}

std::optional<equip_pos> character_dialog::slot_at_paperdoll(int32_t x, int32_t y) const
{
    if (!paperdoll_ || !sprite_mgr_ || !player_ || !inventory_)
        return std::nullopt;

    const auto& spr = player_->sprite();
    return paperdoll_->hit_test(*sprite_mgr_, x, y,
                                paperdoll_anchor_x_, paperdoll_anchor_y_,
                                spr.gender, inventory_);
}

bool character_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (btn == sf::Mouse::Button::Left)
    {
        // Check equipment slots in paperdoll area first
        if (auto slot = slot_at_paperdoll(x, y))
        {
            const item* itm = inventory_ ? inventory_->get_equipped_item(*slot) : nullptr;

            bool is_double_click = (last_clicked_slot_ == slot) && (click_elapsed_ < double_click_threshold_);
            last_clicked_slot_ = slot;
            click_elapsed_ = 0.0f;

            if (is_double_click)
            {
                if (on_double_click_slot_)
                    on_double_click_slot_(*slot);
            }
            else if (itm && on_drag_start_slot_)
            {
                on_drag_start_slot_(*slot, x, y);
            }
            return true;
        }

        // Check buttons
        if (auto idx = button_at(x, y))
        {
            switch (*idx)
            {
            case 0:
                if (on_quest_) on_quest_();
                break;
            case 1:
                if (on_party_) on_party_();
                break;
            case 2:
                if (on_level_settings_) on_level_settings_();
                break;
            }
            return true;
        }

        // Check stat + buttons
        if (stat_points_ > 0)
        {
            if (auto idx = stat_button_at(x, y))
            {
                if (on_add_stat_) on_add_stat_(*idx);
                return true;
            }
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool character_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    hovered_equip_slot_ = slot_at_paperdoll(x, y);

    if (auto idx = button_at(x, y))
        hovered_button_ = *idx;
    else
        hovered_button_ = -1;

    return dialog::handle_mouse_move(x, y);
}

std::optional<int> character_dialog::button_at(int32_t x, int32_t y) const
{
    int32_t btn_y = buttons_y_;
    int32_t total_btn_width = 3 * button_width + 2 * 8;
    int32_t btn_start_x = bounds_.x + (dialog_width - total_btn_width) / 2;

    if (y < btn_y || y > btn_y + button_height)
        return std::nullopt;

    for (int i = 0; i < 3; ++i)
    {
        int32_t bx = btn_start_x + i * (button_width + 8);
        if (x >= bx && x <= bx + button_width)
            return i;
    }

    return std::nullopt;
}

std::optional<int> character_dialog::stat_button_at(int32_t x, int32_t y) const
{
    int32_t row_y = attr_y_;
    int32_t ax = bounds_.x + padding;
    int32_t col_width = (dialog_width - padding * 2) / 2;

    // 2x3 layout: col0row0=0(Str), col1row0=3(Int)
    //             col0row1=1(Vit), col1row1=2(Dex)
    //             col0row2=4(Mag), col1row2=5(Chr)
    static constexpr int stat_indices[3][2] = {{0, 3}, {1, 2}, {4, 5}};

    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            int32_t btn_x = ax + col * col_width + col_width - stat_button_size - 4;
            int32_t btn_y = row_y + row * 20 - 2;

            if (x >= btn_x && x <= btn_x + stat_button_size && y >= btn_y && y <= btn_y + stat_button_size)
            {
                return stat_indices[row][col];
            }
        }
    }

    return std::nullopt;
}

} // namespace hb
