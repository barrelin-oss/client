#include "ui/dialogs/character_dialog.hpp"
#include "entity/components.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>

namespace hb {

character_dialog::character_dialog()
    : dialog(dialog_type::character_info) {
    set_title("Character");
    set_bounds({20, 50, 220, 380});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
    create_ui();
}

void character_dialog::create_ui() {
    // Dialog will be rendered directly, no child widgets needed
}

void character_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
}

void character_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 30;
    int32_t label_width = 90;

    // Level and experience section
    rend.draw_text(std::format("Level: {}", level_), x, y, sf::Color::Yellow);
    y += 20;

    // Experience bar
    rend.draw_text("Exp:", x, y, sf::Color::White);
    int32_t bar_x = x + 40;
    int32_t bar_width = 160;
    rend.draw_rect(bar_x, y, bar_width, 14, sf::Color(40, 40, 50), true);

    float exp_pct = exp_next_ > 0 ? static_cast<float>(exp_) / exp_next_ : 0.0f;
    int32_t fill_width = static_cast<int32_t>(bar_width * exp_pct);
    if (fill_width > 0) {
        rend.draw_rect(bar_x, y, fill_width, 14, sf::Color(100, 150, 200), true);
    }
    rend.draw_rect(bar_x, y, bar_width, 14, sf::Color(80, 80, 100), false);
    y += 22;

    // Separator
    rend.draw_line(x, y, x + 200, y, sf::Color(80, 80, 100));
    y += 8;

    // HP/MP/SP bars
    auto draw_resource_bar = [&](const char* name, int32_t current, int32_t max, sf::Color color) {
        rend.draw_text(name, x, y, sf::Color::White);

        int32_t bar_start = x + 35;
        rend.draw_rect(bar_start, y, bar_width - 35, 14, sf::Color(30, 30, 40), true);

        float pct = max > 0 ? static_cast<float>(current) / max : 0.0f;
        int32_t fw = static_cast<int32_t>((bar_width - 35) * pct);
        if (fw > 0) {
            rend.draw_rect(bar_start, y, fw, 14, color, true);
        }
        rend.draw_rect(bar_start, y, bar_width - 35, 14, sf::Color(60, 60, 80), false);

        std::string text = std::format("{}/{}", current, max);
        int32_t text_x = bar_start + (bar_width - 35) / 2 - static_cast<int32_t>(text.length() * 3);
        rend.draw_text(text, text_x, y, sf::Color::White, 12);
        y += 18;
    };

    draw_resource_bar("HP:", hp_, max_hp_, sf::Color(180, 60, 60));
    draw_resource_bar("MP:", mp_, max_mp_, sf::Color(60, 60, 180));
    draw_resource_bar("SP:", sp_, max_sp_, sf::Color(60, 180, 60));
    y += 6;

    // Separator
    rend.draw_line(x, y, x + 200, y, sf::Color(80, 80, 100));
    y += 8;

    // Base stats section
    rend.draw_text("Base Stats", x, y, sf::Color::Yellow);
    if (stat_points_ > 0) {
        rend.draw_text(std::format("Points: {}", stat_points_),
                      x + 100, y, sf::Color(100, 255, 100));
    }
    y += 22;

    render_stat_row(rend, y, "Strength:", strength_, 0);
    y += 20;
    render_stat_row(rend, y, "Vitality:", vitality_, 1);
    y += 20;
    render_stat_row(rend, y, "Dexterity:", dexterity_, 2);
    y += 20;
    render_stat_row(rend, y, "Intelligence:", intelligence_, 3);
    y += 20;
    render_stat_row(rend, y, "Magic:", magic_, 4);
    y += 20;
    render_stat_row(rend, y, "Charisma:", charisma_, 5);
    y += 26;

    // Separator
    rend.draw_line(x, y, x + 200, y, sf::Color(80, 80, 100));
    y += 8;

    // Combat stats section
    rend.draw_text("Combat Stats", x, y, sf::Color::Yellow);
    y += 22;

    auto draw_combat_stat = [&](const char* name, int32_t value) {
        rend.draw_text(name, x, y, sf::Color::White);
        rend.draw_text(std::to_string(value), x + label_width, y, sf::Color(200, 200, 255));
        y += 18;
    };

    draw_combat_stat("Attack:", attack_power_);
    draw_combat_stat("Magic:", magic_power_);
    draw_combat_stat("Defense:", defense_);
    draw_combat_stat("M.Resist:", magic_resist_);
}

void character_dialog::render_stat_row(renderer& rend, int32_t y, const char* name,
                                       int32_t value, [[maybe_unused]] int32_t stat_index) {
    int32_t x = bounds_.x + 10;

    rend.draw_text(name, x, y, sf::Color::White);
    rend.draw_text(std::to_string(value), x + 90, y, sf::Color(200, 200, 255));

    // Draw + button if stat points available
    if (stat_points_ > 0) {
        int32_t btn_x = x + 130;
        int32_t btn_y = y - 2;

        rend.draw_rect(btn_x, btn_y, stat_button_size, stat_button_size,
                      sf::Color(60, 100, 60), true);
        rend.draw_rect(btn_x, btn_y, stat_button_size, stat_button_size,
                      sf::Color(80, 140, 80), false);
        rend.draw_text("+", btn_x + 4, btn_y, sf::Color::White);
    }
}

void character_dialog::update_stats(const stats_component& stats) {
    strength_ = stats.strength;
    vitality_ = stats.vitality;
    dexterity_ = stats.dexterity;
    intelligence_ = stats.intelligence;
    magic_ = stats.magic;
    charisma_ = stats.charisma;

    hp_ = stats.hp;
    max_hp_ = stats.max_hp;
    mp_ = stats.mp;
    max_mp_ = stats.max_mp;
    sp_ = stats.sp;
    max_sp_ = stats.max_sp;

    level_ = stats.level;
    exp_ = stats.experience;
    exp_next_ = stats.experience_to_next;

    attack_power_ = stats.attack_power;
    magic_power_ = stats.magic_power;
    defense_ = stats.defense;
    magic_resist_ = stats.magic_resist;
}

} // namespace hb
