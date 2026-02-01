#include "ui/dialogs/yaml_icon_panel_dialog.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <spdlog/spdlog.h>

namespace hb {

yaml_icon_panel_dialog::yaml_icon_panel_dialog(dialog_definition def)
    : managed_dialog(std::move(def)) {

    // Wire up button callbacks from YAML action system
    on_button_click("btn_character", [this]() {
        if (on_character_) on_character_();
    });
    on_button_click("btn_inventory", [this]() {
        if (on_inventory_) on_inventory_();
    });
    on_button_click("btn_magic", [this]() {
        if (on_spellbook_) on_spellbook_();
    });
    on_button_click("btn_skills", [this]() {
        if (on_skills_) on_skills_();
    });
    on_button_click("btn_chat", [this]() {
        if (on_chat_history_) on_chat_history_();
    });
    on_button_click("btn_system", [this]() {
        if (on_system_menu_) on_system_menu_();
    });
}

void yaml_icon_panel_dialog::on_update_impl(float delta_time) {
    // Smoothly interpolate bar display values using exponential smoothing
    // Cap the lerp factor to prevent overshooting when dt is large (e.g., first frame)
    float lerp_factor = std::min(1.0f, 8.0f * delta_time);

    float target_hp = hp_max_ > 0 ? static_cast<float>(hp_current_) / hp_max_ : 0.0f;
    float target_mp = mp_max_ > 0 ? static_cast<float>(mp_current_) / mp_max_ : 0.0f;
    float target_sp = sp_max_ > 0 ? static_cast<float>(sp_current_) / sp_max_ : 0.0f;
    float target_exp = exp_to_level_ > 0 ? static_cast<float>(current_exp_) / exp_to_level_ : 0.0f;

    hp_display_ += (target_hp - hp_display_) * lerp_factor;
    mp_display_ += (target_mp - mp_display_) * lerp_factor;
    sp_display_ += (target_sp - sp_display_) * lerp_factor;
    exp_display_ += (target_exp - exp_display_) * lerp_factor;

    hp_display_ = std::clamp(hp_display_, 0.0f, 1.0f);
    mp_display_ = std::clamp(mp_display_, 0.0f, 1.0f);
    sp_display_ = std::clamp(sp_display_, 0.0f, 1.0f);
    exp_display_ = std::clamp(exp_display_, 0.0f, 1.0f);
}

bool yaml_icon_panel_dialog::on_custom_render(renderer& rend) {
    // We handle all rendering ourselves for this dialog
    render_background(rend);

    // Render each custom element based on its type
    for (const auto& elem : definition().elements) {
        auto it = element_states_.find(elem.id);
        if (it == element_states_.end() || !it->second.visible) continue;

        switch (elem.type) {
            case element_type::hp_bar:
                render_hp_bar(rend, elem);
                break;
            case element_type::mp_bar:
                render_mp_bar(rend, elem);
                break;
            case element_type::sp_bar:
                render_sp_bar(rend, elem);
                break;
            case element_type::exp_bar:
                render_exp_bar(rend, elem);
                break;
            case element_type::location_text:
                render_location_text(rend, elem);
                break;
            case element_type::combat_indicator:
                render_combat_indicator(rend, elem);
                break;
            case element_type::super_attack:
                render_super_attack(rend, elem);
                break;
            case element_type::button:
                // Buttons handled by render_buttons for hover state
                break;
            default:
                break;
        }
    }

    // Render buttons last (on top)
    render_buttons(rend);

    return true;  // We handled rendering
}

void yaml_icon_panel_dialog::render_background(renderer& rend) {
    const auto& def = definition();

    // Try classic sprite background first
    if (sprites_ && !def.background_sprite_pak.empty()) {
        const sprite* spr = sprites_->get_sprite(def.background_sprite_pak,
                                                  def.background_sprite_index);

        if (spr && spr->frame_count() > static_cast<uint32_t>(def.background_sprite_frame)) {
            spr->draw(rend.window(), bounds_.x, bounds_.y, def.background_sprite_frame);
            return;
        }
    }

    // Fallback: modern style background
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   sf::Color(20, 20, 30, 220), true);
    rend.draw_rect(bounds_.x, bounds_.y, bounds_.width, 1,
                   sf::Color(60, 60, 80), true);
}

void yaml_icon_panel_dialog::render_hp_bar(renderer& rend, const element_def& elem) {
    ui_bounds abs = get_absolute_bounds(elem);
    // Sprite represents MISSING HP, so we draw (1 - hp_display_) of the width
    float missing = 1.0f - hp_display_;
    int32_t missing_width = static_cast<int32_t>(abs.width * missing);

    const auto& def = definition();

    // Classic sprite rendering - if sprites are available, use them regardless of fill amount
    if (sprites_ && !def.background_sprite_pak.empty()) {
        const sprite* spr = sprites_->get_sprite(def.background_sprite_pak,
                                                  def.background_sprite_index);
        int32_t frame = 12;  // Default for HP/MP bars
        auto frame_it = elem.properties.find("classic_sprite_frame");
        if (frame_it != elem.properties.end()) {
            frame = std::stoi(frame_it->second);
        }

        if (spr && spr->frame_count() > static_cast<uint32_t>(frame)) {
            // Classic mode: draw sprite for MISSING portion (drawn from right side)
            if (missing_width > 0) {
                int32_t x_offset = abs.width - missing_width;
                spr->draw_width(rend.window(), abs.x + x_offset, abs.y, frame, missing_width);

            }

            // Poison overlay tint on the FILLED portion (existing HP)
            if (is_poisoned_) {
                int32_t filled_width = abs.width - missing_width;
                if (filled_width > 0) {
                    sf::RectangleShape poison_overlay(sf::Vector2f(
                        static_cast<float>(filled_width), static_cast<float>(abs.height)
                    ));
                    poison_overlay.setPosition({static_cast<float>(abs.x), static_cast<float>(abs.y)});
                    poison_overlay.setFillColor(sf::Color(0, 150, 0, 128));
                    rend.window().draw(poison_overlay);
                }
            }

            // Classic mode: always draw HP text
            std::string hp_text = std::format("{}/{}", hp_current_, hp_max_);
            int32_t text_x = abs.x + (abs.width - static_cast<int32_t>(hp_text.length()) * 5) / 2;
            rend.draw_text(hp_text, text_x + 1, abs.y + 2, sf::Color(0, 0, 0), 10);
            if (is_poisoned_) {
                rend.draw_text(hp_text, text_x, abs.y + 1, sf::Color(50, 200, 50), 10);
            } else {
                rend.draw_text(hp_text, text_x, abs.y + 1, sf::Color::White, 10);
            }

            if (is_poisoned_) {
                rend.draw_text("Poisoned", abs.x + 5, abs.y + 1, sf::Color(0, 0, 0), 10);
                rend.draw_text("Poisoned", abs.x + 4, abs.y, sf::Color(80, 200, 80), 10);
            }
            return;  // Classic mode complete - never fall through to modern
        }
    }

    // Modern programmatic rendering (only when sprites unavailable)
    // Modern mode shows FILLED portion (unlike classic which shows MISSING)
    int32_t filled_width = abs.width - missing_width;

    sf::Color fill = is_poisoned_ ?
        sf::Color(80, 180, 80) :
        elem.fill_color.value_or(sf::Color(200, 50, 50));
    sf::Color bg = elem.background_color.value_or(sf::Color(60, 20, 20));

    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, bg, true);

    if (filled_width > 0) {
        rend.draw_rect(abs.x, abs.y, filled_width, abs.height, fill, true);

        sf::Color highlight(
            static_cast<uint8_t>(std::min(255, fill.r + 50)),
            static_cast<uint8_t>(std::min(255, fill.g + 50)),
            static_cast<uint8_t>(std::min(255, fill.b + 50))
        );
        rend.draw_rect(abs.x, abs.y, filled_width, 2, highlight, true);
    }

    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, sf::Color(80, 80, 100), false);

    std::string hp_text = std::to_string(hp_current_);
    sf::Color text_color = is_poisoned_ ? sf::Color(100, 200, 100) : sf::Color(200, 100, 100);
    rend.draw_text(hp_text, abs.x + abs.width - 30, abs.y + 3, text_color, 11);

    if (is_poisoned_) {
        rend.draw_text("Poisoned", abs.x + 5, abs.y + 3, sf::Color(80, 200, 80), 10);
    }
}

void yaml_icon_panel_dialog::render_mp_bar(renderer& rend, const element_def& elem) {
    ui_bounds abs = get_absolute_bounds(elem);
    // Sprite represents MISSING MP
    float missing = 1.0f - mp_display_;
    int32_t missing_width = static_cast<int32_t>(abs.width * missing);

    const auto& def = definition();

    // Classic sprite rendering
    if (sprites_ && !def.background_sprite_pak.empty()) {
        const sprite* spr = sprites_->get_sprite(def.background_sprite_pak,
                                                  def.background_sprite_index);
        int32_t frame = 12;  // Default for HP/MP bars
        auto frame_it = elem.properties.find("classic_sprite_frame");
        if (frame_it != elem.properties.end()) {
            frame = std::stoi(frame_it->second);
        }

        if (spr && spr->frame_count() > static_cast<uint32_t>(frame)) {
            // Draw sprite for MISSING portion (from right side)
            if (missing_width > 0) {
                int32_t x_offset = abs.width - missing_width;
                spr->draw_width(rend.window(), abs.x + x_offset, abs.y, frame, missing_width);
            }

            // Always draw MP text in classic style
            std::string mp_text = std::format("{}/{}", mp_current_, mp_max_);
            int32_t text_x = abs.x + (abs.width - static_cast<int32_t>(mp_text.length()) * 5) / 2;
            rend.draw_text(mp_text, text_x + 1, abs.y + 2, sf::Color(0, 0, 0), 10);
            rend.draw_text(mp_text, text_x, abs.y + 1, sf::Color::White, 10);
            return;  // Classic mode complete
        }
    }

    // Modern programmatic rendering (shows FILLED portion)
    int32_t filled_width = abs.width - missing_width;
    sf::Color fill = elem.fill_color.value_or(sf::Color(50, 100, 200));
    sf::Color bg = elem.background_color.value_or(sf::Color(20, 40, 80));

    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, bg, true);

    if (filled_width > 0) {
        rend.draw_rect(abs.x, abs.y, filled_width, abs.height, fill, true);
        sf::Color highlight(90, 140, 240);
        rend.draw_rect(abs.x, abs.y, filled_width, 2, highlight, true);
    }

    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, sf::Color(80, 80, 100), false);

    std::string mp_text = std::to_string(mp_current_);
    rend.draw_text(mp_text, abs.x + abs.width - 30, abs.y + 3, sf::Color(100, 100, 200), 11);
}

void yaml_icon_panel_dialog::render_sp_bar(renderer& rend, const element_def& elem) {
    ui_bounds abs = get_absolute_bounds(elem);
    // Sprite represents MISSING SP
    float missing = 1.0f - sp_display_;
    int32_t missing_width = static_cast<int32_t>(abs.width * missing);

    const auto& def = definition();

    // Classic sprite rendering
    if (sprites_ && !def.background_sprite_pak.empty()) {
        const sprite* spr = sprites_->get_sprite(def.background_sprite_pak,
                                                  def.background_sprite_index);
        int32_t frame = 13;  // Default for SP bar
        auto frame_it = elem.properties.find("classic_sprite_frame");
        if (frame_it != elem.properties.end()) {
            frame = std::stoi(frame_it->second);
        }

        if (spr && spr->frame_count() > static_cast<uint32_t>(frame)) {
            // Draw sprite for MISSING portion (from right side)
            if (missing_width > 0) {
                int32_t x_offset = abs.width - missing_width;
                spr->draw_width(rend.window(), abs.x + x_offset, abs.y, frame, missing_width);
            }
            return;  // Classic mode complete
        }
    }

    // Modern programmatic rendering (shows FILLED portion)
    int32_t filled_width = abs.width - missing_width;
    sf::Color fill = elem.fill_color.value_or(sf::Color(200, 180, 50));
    sf::Color bg = elem.background_color.value_or(sf::Color(80, 70, 20));

    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, bg, true);

    if (filled_width > 0) {
        rend.draw_rect(abs.x, abs.y, filled_width, abs.height, fill, true);
        sf::Color highlight(240, 220, 90);
        rend.draw_rect(abs.x, abs.y, filled_width, 2, highlight, true);
    }

    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, sf::Color(80, 80, 100), false);

    rend.draw_text("SP", abs.x + 2, abs.y, sf::Color(180, 160, 80), 10);
}

void yaml_icon_panel_dialog::render_exp_bar(renderer& rend, const element_def& elem) {
    ui_bounds abs = get_absolute_bounds(elem);

    sf::Color fill = elem.fill_color.value_or(sf::Color(100, 200, 100));
    sf::Color bg = elem.background_color.value_or(sf::Color(30, 60, 30));

    // Background
    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, bg, true);

    // Fill
    int32_t fill_width = static_cast<int32_t>(abs.width * exp_display_);
    if (fill_width > 0) {
        rend.draw_rect(abs.x, abs.y, fill_width, abs.height, fill, true);
        sf::Color highlight(140, 240, 140);
        rend.draw_rect(abs.x, abs.y, fill_width, 2, highlight, true);
    }

    // Border
    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, sf::Color(80, 80, 100), false);

    // Percentage text
    // Classic mode does not show percentage on EXP bar
    //int32_t exp_percent = exp_to_level_ > 0 ?
    //    static_cast<int32_t>((current_exp_ * 100) / exp_to_level_) : 0;
    //std::string exp_text = std::format("{}%", exp_percent);
    //rend.draw_text(exp_text, abs.x + abs.width - 25, abs.y, sf::Color(120, 180, 120), 9);
}

void yaml_icon_panel_dialog::render_location_text(renderer& rend, const element_def& elem) {
    ui_bounds abs = get_absolute_bounds(elem);

    std::string display_text;

    if (mouse_in_info_area_) {
        // Show experience info when hovering
        if (exp_to_level_ > 0) {
            int64_t exp_remaining = exp_to_level_ - current_exp_;
            if (exp_remaining < 0) exp_remaining = 0;

            int32_t percent = static_cast<int32_t>((current_exp_ * 10000) / exp_to_level_);
            display_text = std::format("EXP {}/{} ({}.{:02}%)",
                                        exp_remaining, exp_to_level_,
                                        percent / 100, percent % 100);
        } else {
            display_text = std::format("EXP {}", current_exp_);
        }
    } else {
        // Show map name and coordinates
        display_text = std::format("{}({},{})", map_name_, player_x_, player_y_);
    }

    // Center the text
    int32_t text_width = static_cast<int32_t>(display_text.length()) * 6;
    int32_t centered_x = abs.x + (abs.width - text_width) / 2;

    sf::Color color = elem.text_color.value_or(sf::Color(200, 200, 120));
    rend.draw_text(display_text, centered_x, abs.y + 10, color, elem.font_size.value_or(11));
}

void yaml_icon_panel_dialog::render_combat_indicator(renderer& rend, const element_def& elem) {
    // Determine which frame to show (or none)
    // Frame 3: Alt held + weapon mastered (super attack ready)
    // Frame 4: Safe attack mode (combat + safe)
    // Frame 5: Attack mode (combat + not safe / PK mode)
    // No frame: Default stance (not in combat mode)

    int32_t frame = -1;  // -1 means don't render

    if (alt_held_ && super_attack_available_) {
        frame = 3;
    } else if (combat_mode_ && safe_attack_mode_) {
        frame = 4;
    } else if (combat_mode_ && !safe_attack_mode_) {
        frame = 5;
    }
    // else: default stance, no sprite

    if (frame < 0) return;  // Nothing to render

    ui_bounds abs = get_absolute_bounds(elem);
    const auto& def = definition();

    // Classic sprite rendering
    if (sprites_ && !def.background_sprite_pak.empty()) {
        const sprite* spr = sprites_->get_sprite(def.background_sprite_pak,
                                                  def.background_sprite_index);
        if (spr && spr->frame_count() > static_cast<uint32_t>(frame)) {
            spr->draw(rend.window(), abs.x, abs.y, frame);
            return;
        }
    }

    // Modern fallback (colored rectangles)
    sf::Color indicator_color;
    const char* mode_text;

    if (frame == 3) {
        indicator_color = sf::Color(200, 150, 50);  // Gold for super attack
        mode_text = "!";
    } else if (frame == 4) {
        indicator_color = sf::Color(100, 200, 100);  // Green for safe
        mode_text = "S";
    } else {
        indicator_color = sf::Color(200, 100, 100);  // Red for PK
        mode_text = "P";
    }

    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, indicator_color, true);
    rend.draw_rect(abs.x, abs.y, abs.width, abs.height, sf::Color(150, 150, 150), false);
    rend.draw_text(mode_text, abs.x + 10, abs.y + 8, sf::Color::White, 12);
}

void yaml_icon_panel_dialog::render_super_attack(renderer& rend, const element_def& elem) {
    if (super_attack_count_ <= 0) return;

    ui_bounds abs = get_absolute_bounds(elem);

    // Highlight if super attack is available
    if (super_attack_available_) {
        rend.draw_rect(abs.x, abs.y, abs.width, abs.height,
                       sf::Color(200, 150, 50, 150), true);
    }

    // Counter text
    std::string count_text = std::to_string(super_attack_count_);
    sf::Color count_color = super_attack_available_ ?
        sf::Color(220, 200, 200) : sf::Color(80, 80, 80);
    rend.draw_text(count_text, abs.x + 15, abs.y + 20, count_color, 11);
}

void yaml_icon_panel_dialog::render_buttons(renderer& rend) {
    for (const auto& elem : definition().elements) {
        if (elem.type != element_type::button) continue;

        auto it = element_states_.find(elem.id);
        if (it == element_states_.end() || !it->second.visible) continue;

        bool hovered = it->second.hovered;

        // Classic mode: render nothing unless hovered
        if (sprites_ && !elem.sprite_pak.empty()) {
            if (!hovered) continue;  // Don't render anything when not hovered

            const sprite* spr = sprites_->get_sprite(elem.sprite_pak, elem.sprite_index);
            if (spr) {
                ui_bounds abs = get_absolute_bounds(elem);
                int32_t frame = elem.sprite_frame;

                // Draw the button sprite on hover
                if (static_cast<uint32_t>(frame) < spr->frame_count()) {
                    spr->draw(rend.window(), abs.x, abs.y, frame);
                }
                continue;
            }
        }

        // Modern fallback: only render when hovered
        if (!hovered) continue;

        ui_bounds abs = get_absolute_bounds(elem);
        sf::Color bg = sf::Color(70, 70, 90, 220);
        rend.draw_rect(abs.x, abs.y, abs.width, abs.height, bg, true);
        rend.draw_rect(abs.x, abs.y, abs.width, abs.height, sf::Color(80, 80, 100), false);

        sf::Color text_color = sf::Color::White;
        rend.draw_text(elem.text, abs.x + abs.width / 2 - 4, abs.y + abs.height / 2 - 6,
                       text_color, 12);

        // Tooltip when hovered
        if (hovered && !elem.tooltip.empty()) {
            int32_t tooltip_y = bounds_.y - 22;
            int32_t tooltip_width = static_cast<int32_t>(elem.tooltip.length()) * 7 + 8;

            rend.draw_rect(abs.x, tooltip_y, tooltip_width, 18,
                           sf::Color(30, 30, 40, 240), true);
            rend.draw_rect(abs.x, tooltip_y, tooltip_width, 18,
                           sf::Color(80, 80, 100), false);
            rend.draw_text(elem.tooltip, abs.x + 4, tooltip_y + 2,
                           sf::Color(200, 200, 120), 11);
        }
    }
}

bool yaml_icon_panel_dialog::on_custom_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (btn != sf::Mouse::Button::Left) {
        return false;
    }

    // Check if clicking on the combat indicator
    for (const auto& elem : definition().elements) {
        if (elem.id == "combat_indicator") {
            ui_bounds abs = get_absolute_bounds(elem);
            if (abs.contains(x, y)) {
                if (on_combat_indicator_) {
                    on_combat_indicator_();
                }
                return true;
            }
            break;
        }
    }

    return false;
}

int32_t yaml_icon_panel_dialog::get_hovered_button(int32_t x, int32_t y) const {
    int32_t index = 0;
    for (const auto& elem : definition().elements) {
        if (elem.type == element_type::button) {
            ui_bounds abs = get_absolute_bounds(elem);
            if (abs.contains(x, y)) {
                return index;
            }
            ++index;
        }
    }
    return -1;
}

// === Setters ===

void yaml_icon_panel_dialog::set_hp(int32_t current, int32_t max) {
    hp_current_ = std::max(0, current);
    hp_max_ = std::max(1, max);
}

void yaml_icon_panel_dialog::set_mp(int32_t current, int32_t max) {
    mp_current_ = std::max(0, current);
    mp_max_ = std::max(1, max);
}

void yaml_icon_panel_dialog::set_sp(int32_t current, int32_t max) {
    sp_current_ = std::max(0, current);
    sp_max_ = std::max(1, max);
}

void yaml_icon_panel_dialog::set_experience(int64_t current_exp, int64_t exp_to_level, int32_t level) {
    current_exp_ = std::max(static_cast<int64_t>(0), current_exp);
    exp_to_level_ = std::max(static_cast<int64_t>(1), exp_to_level);
    player_level_ = level;
}

void yaml_icon_panel_dialog::set_map_name(std::string_view name) {
    map_name_ = name;
}

void yaml_icon_panel_dialog::set_position(int32_t x, int32_t y) {
    player_x_ = x;
    player_y_ = y;
}

void yaml_icon_panel_dialog::set_combat_mode(bool combat) {
    combat_mode_ = combat;
}

void yaml_icon_panel_dialog::set_safe_attack_mode(bool safe) {
    safe_attack_mode_ = safe;
}

void yaml_icon_panel_dialog::set_super_attack_count(int32_t count) {
    super_attack_count_ = count;
}

void yaml_icon_panel_dialog::set_super_attack_available(bool available) {
    super_attack_available_ = available;
}

void yaml_icon_panel_dialog::set_alt_held(bool held) {
    alt_held_ = held;
}

void yaml_icon_panel_dialog::set_poisoned(bool poisoned) {
    is_poisoned_ = poisoned;
}

} // namespace hb
