#include "ui/dialogs/icon_panel_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include "assets/sprite_manager.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <spdlog/spdlog.h>

namespace hb {

icon_panel_dialog::icon_panel_dialog()
    : dialog(dialog_type::icon_panel) {
    // Position at bottom of screen, full width
    set_title("");
    set_bounds({0, panel_y, static_cast<int32_t>(screen_width), panel_height});
    set_closeable(false);
    set_draggable(false);
    set_modal(false);
    set_has_border(false);
    set_background_color(sf::Color(0, 0, 0, 0));  // Transparent - we draw custom background
}

void icon_panel_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;

    // Smoothly interpolate bar display values
    float lerp_speed = 8.0f * delta_time;

    float target_hp = hp_max_ > 0 ? static_cast<float>(hp_current_) / hp_max_ : 0.0f;
    float target_mp = mp_max_ > 0 ? static_cast<float>(mp_current_) / mp_max_ : 0.0f;
    float target_sp = sp_max_ > 0 ? static_cast<float>(sp_current_) / sp_max_ : 0.0f;
    float target_exp = exp_to_level_ > 0 ? static_cast<float>(current_exp_) / exp_to_level_ : 0.0f;

    hp_display_ += (target_hp - hp_display_) * lerp_speed;
    mp_display_ += (target_mp - mp_display_) * lerp_speed;
    sp_display_ += (target_sp - sp_display_) * lerp_speed;
    exp_display_ += (target_exp - exp_display_) * lerp_speed;

    hp_display_ = std::clamp(hp_display_, 0.0f, 1.0f);
    mp_display_ = std::clamp(mp_display_, 0.0f, 1.0f);
    sp_display_ = std::clamp(sp_display_, 0.0f, 1.0f);
    exp_display_ = std::clamp(exp_display_, 0.0f, 1.0f);

    // Update hovered button
    int32_t mx = inp.mouse_x();
    int32_t my = inp.mouse_y();
    hovered_button_ = get_hovered_button(mx, my);

    // Check if mouse is in info area (show exp instead of map info)
    mouse_in_info_area_ = (mx >= info_area_x && mx < info_area_x + info_area_width &&
                           my >= panel_y + info_area_y && my < panel_y + info_area_y + info_area_height);
}

void icon_panel_dialog::render(renderer& rend) {
    if (!visible_) return;

    // Choose rendering style
    if (ui_style_ == ui_style::classic && sprites_ != nullptr) {
        render_classic(rend);
    } else {
        render_modern(rend);
    }
}

// =============================================================================
// Modern Style Rendering
// =============================================================================

void icon_panel_dialog::render_modern(renderer& rend) {
    // Draw panel background (dark semi-transparent bar)
    rend.draw_rect(0, panel_y, static_cast<int32_t>(screen_width), panel_height,
                   sf::Color(20, 20, 30, 220), true);

    // Draw subtle top border
    rend.draw_rect(0, panel_y, static_cast<int32_t>(screen_width), 1,
                   sf::Color(60, 60, 80), true);

    // Render components
    render_modern_gauge_bars(rend);
    render_modern_combat_indicator(rend);
    render_modern_super_attack_counter(rend);
    render_modern_map_info(rend);
    render_modern_action_buttons(rend);
}

void icon_panel_dialog::render_modern_gauge_bars(renderer& rend) {
    // HP bar colors
    sf::Color hp_fill = is_poisoned_ ? sf::Color(80, 180, 80) : sf::Color(200, 50, 50);
    sf::Color hp_bg = sf::Color(60, 20, 20);

    // HP Bar
    int32_t hp_y = panel_y + hp_bar_y;
    rend.draw_rect(hp_bar_x, hp_y, hp_mp_bar_width, hp_mp_bar_height, hp_bg, true);

    int32_t hp_fill_width = static_cast<int32_t>(hp_mp_bar_width * hp_display_);
    if (hp_fill_width > 0) {
        rend.draw_rect(hp_bar_x, hp_y, hp_fill_width, hp_mp_bar_height, hp_fill, true);
        // Highlight at top
        sf::Color hp_highlight(
            static_cast<uint8_t>(std::min(255, hp_fill.r + 50)),
            static_cast<uint8_t>(std::min(255, hp_fill.g + 50)),
            static_cast<uint8_t>(std::min(255, hp_fill.b + 50))
        );
        rend.draw_rect(hp_bar_x, hp_y, hp_fill_width, 2, hp_highlight, true);
    }
    rend.draw_rect(hp_bar_x, hp_y, hp_mp_bar_width, hp_mp_bar_height, sf::Color(80, 80, 100), false);

    // HP value
    std::string hp_text = std::to_string(hp_current_);
    sf::Color hp_text_color = is_poisoned_ ? sf::Color(100, 200, 100) : sf::Color(200, 100, 100);
    rend.draw_text(hp_text, hp_bar_x + hp_mp_bar_width - 30, hp_y + 3, hp_text_color, 11);

    // If poisoned, show text
    if (is_poisoned_) {
        rend.draw_text("Poisoned", hp_bar_x + 5, hp_y + 3, sf::Color(80, 200, 80), 10);
    }

    // MP Bar
    int32_t mp_y = panel_y + mp_bar_y;
    sf::Color mp_fill = sf::Color(50, 100, 200);
    sf::Color mp_bg = sf::Color(20, 40, 80);

    rend.draw_rect(hp_bar_x, mp_y, hp_mp_bar_width, hp_mp_bar_height, mp_bg, true);

    int32_t mp_fill_width = static_cast<int32_t>(hp_mp_bar_width * mp_display_);
    if (mp_fill_width > 0) {
        rend.draw_rect(hp_bar_x, mp_y, mp_fill_width, hp_mp_bar_height, mp_fill, true);
        sf::Color mp_highlight(90, 140, 240);
        rend.draw_rect(hp_bar_x, mp_y, mp_fill_width, 2, mp_highlight, true);
    }
    rend.draw_rect(hp_bar_x, mp_y, hp_mp_bar_width, hp_mp_bar_height, sf::Color(80, 80, 100), false);

    // MP value
    std::string mp_text = std::to_string(mp_current_);
    rend.draw_text(mp_text, hp_bar_x + hp_mp_bar_width - 30, mp_y + 3, sf::Color(100, 100, 200), 11);

    // SP Bar (Stamina - horizontal bar in middle section)
    int32_t sp_y = panel_y + sp_bar_y;
    sf::Color sp_fill = sf::Color(200, 180, 50);
    sf::Color sp_bg = sf::Color(80, 70, 20);

    rend.draw_rect(sp_bar_x, sp_y, sp_bar_width, sp_bar_height, sp_bg, true);

    int32_t sp_fill_width = static_cast<int32_t>(sp_bar_width * sp_display_);
    if (sp_fill_width > 0) {
        rend.draw_rect(sp_bar_x, sp_y, sp_fill_width, sp_bar_height, sp_fill, true);
        sf::Color sp_highlight(240, 220, 90);
        rend.draw_rect(sp_bar_x, sp_y, sp_fill_width, 2, sp_highlight, true);
    }
    rend.draw_rect(sp_bar_x, sp_y, sp_bar_width, sp_bar_height, sf::Color(80, 80, 100), false);

    // SP label
    rend.draw_text("SP", sp_bar_x + 2, sp_y, sf::Color(180, 160, 80), 10);

    // EXP Bar (below SP bar)
    int32_t exp_y = panel_y + exp_bar_y;
    sf::Color exp_fill = sf::Color(100, 200, 100);
    sf::Color exp_bg = sf::Color(30, 60, 30);

    rend.draw_rect(exp_bar_x, exp_y, exp_bar_width, exp_bar_height, exp_bg, true);

    int32_t exp_fill_width = static_cast<int32_t>(exp_bar_width * exp_display_);
    if (exp_fill_width > 0) {
        rend.draw_rect(exp_bar_x, exp_y, exp_fill_width, exp_bar_height, exp_fill, true);
        sf::Color exp_highlight(140, 240, 140);
        rend.draw_rect(exp_bar_x, exp_y, exp_fill_width, 2, exp_highlight, true);
    }
    rend.draw_rect(exp_bar_x, exp_y, exp_bar_width, exp_bar_height, sf::Color(80, 80, 100), false);

    // EXP percentage
    int32_t exp_percent = exp_to_level_ > 0 ? static_cast<int32_t>((current_exp_ * 100) / exp_to_level_) : 0;
    std::string exp_text = std::format("{}%", exp_percent);
    rend.draw_text(exp_text, exp_bar_x + exp_bar_width - 25, exp_y, sf::Color(120, 180, 120), 9);
}

void icon_panel_dialog::render_modern_map_info(renderer& rend) {
    int32_t text_y = panel_y + info_area_y;
    int32_t text_x = info_area_x + info_area_width / 2;  // Center position

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

    // Calculate text width for centering (approximate)
    int32_t text_width = static_cast<int32_t>(display_text.length()) * 6;
    int32_t centered_x = text_x - text_width / 2;

    rend.draw_text(display_text, centered_x, text_y, sf::Color(200, 200, 120), 11);
}

void icon_panel_dialog::render_modern_action_buttons(renderer& rend) {
    static const char* button_labels[] = {
        "C",    // Character (F5)
        "I",    // Inventory (F6)
        "M",    // Magic/Spellbook (F7)
        "K",    // Skills (F8)
        "H",    // Chat History (F9)
        "S"     // System Menu (F12)
    };

    static const char* button_tooltips[] = {
        "Character (F5)",
        "Inventory (F6)",
        "Magic (F7)",
        "Skills (F8)",
        "History (F9)",
        "System (F12)"
    };

    for (int32_t i = 0; i < button_count; ++i) {
        int32_t btn_x = button_start_x + i * button_width;
        int32_t btn_y = panel_y + button_y;

        // Button background
        sf::Color bg_color = (hovered_button_ == i) ?
            sf::Color(70, 70, 90, 220) : sf::Color(40, 40, 55, 200);

        rend.draw_rect(btn_x, btn_y, button_width, button_height, bg_color, true);
        rend.draw_rect(btn_x, btn_y, button_width, button_height, sf::Color(80, 80, 100), false);

        // Button label
        sf::Color text_color = (hovered_button_ == i) ?
            sf::Color::White : sf::Color(180, 180, 200);
        rend.draw_text(button_labels[i], btn_x + button_width / 2 - 4, btn_y + button_height / 2 - 6,
                       text_color, 12);

        // Tooltip when hovered
        if (hovered_button_ == i) {
            int32_t tooltip_x = btn_x;
            int32_t tooltip_y = panel_y - 22;
            int32_t tooltip_width = static_cast<int32_t>(strlen(button_tooltips[i])) * 7 + 8;

            rend.draw_rect(tooltip_x, tooltip_y, tooltip_width, 18,
                           sf::Color(30, 30, 40, 240), true);
            rend.draw_rect(tooltip_x, tooltip_y, tooltip_width, 18,
                           sf::Color(80, 80, 100), false);
            rend.draw_text(button_tooltips[i], tooltip_x + 4, tooltip_y + 2,
                           sf::Color(200, 200, 120), 11);
        }
    }
}

void icon_panel_dialog::render_modern_combat_indicator(renderer& rend) {
    if (!combat_mode_) return;

    int32_t cx = combat_x;
    int32_t cy = panel_y + combat_y;

    // Combat mode indicator
    sf::Color indicator_color = safe_attack_mode_ ?
        sf::Color(100, 200, 100) :  // Green for safe mode
        sf::Color(200, 100, 100);   // Red for PK mode

    rend.draw_rect(cx, cy, 30, 30, indicator_color, true);
    rend.draw_rect(cx, cy, 30, 30, sf::Color(150, 150, 150), false);

    // Icon text
    const char* mode_text = safe_attack_mode_ ? "S" : "P";
    rend.draw_text(mode_text, cx + 10, cy + 8, sf::Color::White, 12);
}

void icon_panel_dialog::render_modern_super_attack_counter(renderer& rend) {
    if (super_attack_count_ <= 0) return;

    int32_t sx = super_attack_x;
    int32_t sy = panel_y + super_attack_y;

    // Only highlight if super attack is available (100% mastery)
    if (super_attack_available_) {
        // Animated highlight effect
        rend.draw_rect(sx, sy, 40, 40, sf::Color(200, 150, 50, 150), true);
    }

    // Counter text
    std::string count_text = std::to_string(super_attack_count_);
    sf::Color count_color = super_attack_available_ ?
        sf::Color(220, 200, 200) : sf::Color(80, 80, 80);
    rend.draw_text(count_text, sx + 15, sy + 20, count_color, 11);
}

// =============================================================================
// Classic Style Rendering (Sprite-based)
// =============================================================================

void icon_panel_dialog::render_classic(renderer& rend) {
    render_classic_background(rend);
    render_classic_gauge_bars(rend);
    render_classic_map_info(rend);
    render_classic_action_buttons(rend);
}

void icon_panel_dialog::render_classic_background(renderer& rend) {
    // Try to render the classic panel background sprite from GameDialog.pak
    const sprite* icon_spr = sprites_ ?
        sprites_->get_sprite(classic_sprites::pak_name, classic_sprites::sprite_index) : nullptr;

    if (icon_spr && icon_spr->frame_count() > classic_sprites::panel_background) {
        // Draw the panel background sprite (frame 14)
        icon_spr->draw(rend.window(), 0, panel_y, classic_sprites::panel_background);
    } else {
        // Fallback: programmatic classic-style background
        rend.draw_rect(0, panel_y, static_cast<int32_t>(screen_width), panel_height,
                       sf::Color(32, 32, 48, 255), true);

        // Classic border styling
        rend.draw_line(0, panel_y, static_cast<int32_t>(screen_width), panel_y,
                       sf::Color(100, 100, 140));
        rend.draw_line(0, panel_y + panel_height - 1, static_cast<int32_t>(screen_width), panel_y + panel_height - 1,
                       sf::Color(20, 20, 30));
    }
}

void icon_panel_dialog::render_classic_gauge_bars(renderer& rend) {
    // Get the icon panel sprite for bar fills
    const sprite* icon_spr = sprites_ ?
        sprites_->get_sprite(classic_sprites::pak_name, classic_sprites::sprite_index) : nullptr;

    // HP bar (position from legacy: 23, 437)
    int32_t hp_y = classic_layout::hp_bar_y;
    int32_t hp_fill_width = static_cast<int32_t>(classic_layout::bar_max_width * hp_display_);

    // Draw HP bar fill using sprite if available
    if (icon_spr && icon_spr->frame_count() > classic_sprites::hp_mp_bar_fill && hp_fill_width > 0) {
        // Use sprite-based variable-width rendering (like legacy PutSpriteFastWidth)
        icon_spr->draw_width(rend.window(), classic_layout::hp_bar_x, hp_y,
                             classic_sprites::hp_mp_bar_fill, hp_fill_width);

        // If poisoned, tint the bar green (draw overlay)
        if (is_poisoned_) {
            sf::RectangleShape poison_overlay(sf::Vector2f(
                static_cast<float>(hp_fill_width),
                14.0f
            ));
            poison_overlay.setPosition({
                static_cast<float>(classic_layout::hp_bar_x),
                static_cast<float>(hp_y)
            });
            poison_overlay.setFillColor(sf::Color(0, 150, 0, 128));
            rend.window().draw(poison_overlay);
        }
    } else {
        // Fallback: programmatic rendering
        rend.draw_rect(classic_layout::hp_bar_x, hp_y, classic_layout::bar_max_width, 14,
                       sf::Color(40, 0, 0), true);
        if (hp_fill_width > 0) {
            sf::Color hp_color = is_poisoned_ ? sf::Color(0, 150, 0) : sf::Color(180, 0, 0);
            rend.draw_rect(classic_layout::hp_bar_x, hp_y, hp_fill_width, 14, hp_color, true);
            sf::Color hp_light = is_poisoned_ ? sf::Color(0, 200, 0) : sf::Color(220, 50, 50);
            rend.draw_rect(classic_layout::hp_bar_x, hp_y, hp_fill_width, 3, hp_light, true);
        }
    }

    // HP text centered
    std::string hp_str = std::format("{}/{}", hp_current_, hp_max_);
    int32_t hp_text_x = classic_layout::hp_bar_x + (classic_layout::bar_max_width - static_cast<int32_t>(hp_str.length()) * 5) / 2;
    rend.draw_text(hp_str, hp_text_x + 1, hp_y + 2, sf::Color(0, 0, 0), 10);  // Shadow
    rend.draw_text(hp_str, hp_text_x, hp_y + 1, sf::Color::White, 10);

    // MP bar (position from legacy: 23, 459)
    int32_t mp_y = classic_layout::mp_bar_y;
    int32_t mp_fill_width = static_cast<int32_t>(classic_layout::bar_max_width * mp_display_);

    // Draw MP bar fill using sprite if available
    if (icon_spr && icon_spr->frame_count() > classic_sprites::hp_mp_bar_fill && mp_fill_width > 0) {
        icon_spr->draw_width(rend.window(), classic_layout::hp_bar_x, mp_y,
                             classic_sprites::hp_mp_bar_fill, mp_fill_width);
    } else {
        // Fallback: programmatic rendering
        rend.draw_rect(classic_layout::hp_bar_x, mp_y, classic_layout::bar_max_width, 14,
                       sf::Color(0, 0, 40), true);
        if (mp_fill_width > 0) {
            rend.draw_rect(classic_layout::hp_bar_x, mp_y, mp_fill_width, 14,
                           sf::Color(0, 0, 180), true);
            rend.draw_rect(classic_layout::hp_bar_x, mp_y, mp_fill_width, 3, sf::Color(50, 50, 220), true);
        }
    }

    // MP text centered
    std::string mp_str = std::format("{}/{}", mp_current_, mp_max_);
    int32_t mp_text_x = classic_layout::hp_bar_x + (classic_layout::bar_max_width - static_cast<int32_t>(mp_str.length()) * 5) / 2;
    rend.draw_text(mp_str, mp_text_x + 1, mp_y + 2, sf::Color(0, 0, 0), 10);  // Shadow
    rend.draw_text(mp_str, mp_text_x, mp_y + 1, sf::Color::White, 10);

    // SP bar - yellow/gold (position from legacy: 147, 435)
    int32_t sp_y = classic_layout::sp_bar_y;
    int32_t sp_fill_width = static_cast<int32_t>(classic_layout::sp_bar_max_width * sp_display_);

    // Draw SP bar fill using sprite if available
    if (icon_spr && icon_spr->frame_count() > classic_sprites::sp_bar_fill && sp_fill_width > 0) {
        icon_spr->draw_width(rend.window(), classic_layout::sp_bar_x, sp_y,
                             classic_sprites::sp_bar_fill, sp_fill_width);
    } else {
        // Fallback: programmatic rendering
        rend.draw_rect(classic_layout::sp_bar_x, sp_y, classic_layout::sp_bar_max_width, 12,
                       sf::Color(40, 35, 0), true);
        if (sp_fill_width > 0) {
            rend.draw_rect(classic_layout::sp_bar_x, sp_y, sp_fill_width, 12,
                           sf::Color(180, 160, 0), true);
            rend.draw_rect(classic_layout::sp_bar_x, sp_y, sp_fill_width, 2, sf::Color(220, 200, 50), true);
        }
    }
}

void icon_panel_dialog::render_classic_map_info(renderer& rend) {
    int32_t text_y = panel_y + info_area_y;
    int32_t text_x = info_area_x + info_area_width / 2;

    // Classic styling uses brighter yellow text
    std::string display_text = std::format("{}({},{})", map_name_, player_x_, player_y_);

    int32_t text_width = static_cast<int32_t>(display_text.length()) * 6;
    int32_t centered_x = text_x - text_width / 2;

    // Classic shadow effect
    rend.draw_text(display_text, centered_x + 1, text_y + 1, sf::Color(0, 0, 0), 11);
    rend.draw_text(display_text, centered_x, text_y, sf::Color(255, 255, 180), 11);
}

void icon_panel_dialog::render_classic_action_buttons(renderer& rend) {
    // Classic button sprites from GameDialog.pak
    const sprite* icon_spr = sprites_ ?
        sprites_->get_sprite(classic_sprites::pak_name, classic_sprites::sprite_index) : nullptr;

    // Button frame indices and X positions (using classic_layout values)
    static const uint32_t button_frames[] = {
        classic_sprites::button_character,  // Frame 6
        classic_sprites::button_inventory,  // Frame 7
        classic_sprites::button_magic,      // Frame 8
        classic_sprites::button_skills,     // Frame 9
        classic_sprites::button_chat,       // Frame 10
        classic_sprites::button_system      // Frame 11
    };

    static const int32_t button_x_positions[] = {
        classic_layout::button_character_x,
        classic_layout::button_inventory_x,
        classic_layout::button_magic_x,
        classic_layout::button_skills_x,
        classic_layout::button_chat_x,
        classic_layout::button_system_x
    };

    // Fallback labels if sprites not available
    static const char* button_labels[] = { "C", "I", "M", "K", "H", "S" };

    for (int32_t i = 0; i < button_count; ++i) {
        int32_t btn_x = button_x_positions[i];
        int32_t btn_y = classic_layout::button_y;
        bool hovered = (hovered_button_ == i);

        // Try to render sprite button
        if (icon_spr && icon_spr->frame_count() > button_frames[i]) {
            icon_spr->draw(rend.window(), btn_x, btn_y, button_frames[i]);

            // Draw hover highlight effect if hovered
            if (hovered && icon_spr->frame_count() > classic_sprites::panel_hover_highlight) {
                icon_spr->draw_alpha(rend.window(), btn_x, btn_y,
                                     classic_sprites::panel_hover_highlight, 0.5f);
            }
        } else {
            // Fallback: programmatic classic-style buttons
            sf::Color base = hovered ? sf::Color(80, 80, 100) : sf::Color(50, 50, 70);
            rend.draw_rect(btn_x, btn_y, classic_layout::button_width, classic_layout::button_height, base, true);

            // Classic bevel effect
            if (!hovered) {
                rend.draw_line(btn_x, btn_y, btn_x + classic_layout::button_width - 1, btn_y, sf::Color(100, 100, 130));
                rend.draw_line(btn_x, btn_y, btn_x, btn_y + classic_layout::button_height - 1, sf::Color(100, 100, 130));
                rend.draw_line(btn_x + classic_layout::button_width - 1, btn_y, btn_x + classic_layout::button_width - 1, btn_y + classic_layout::button_height - 1, sf::Color(30, 30, 40));
                rend.draw_line(btn_x, btn_y + classic_layout::button_height - 1, btn_x + classic_layout::button_width - 1, btn_y + classic_layout::button_height - 1, sf::Color(30, 30, 40));
            } else {
                rend.draw_line(btn_x, btn_y, btn_x + classic_layout::button_width - 1, btn_y, sf::Color(30, 30, 40));
                rend.draw_line(btn_x, btn_y, btn_x, btn_y + classic_layout::button_height - 1, sf::Color(30, 30, 40));
                rend.draw_line(btn_x + classic_layout::button_width - 1, btn_y, btn_x + classic_layout::button_width - 1, btn_y + classic_layout::button_height - 1, sf::Color(120, 120, 150));
                rend.draw_line(btn_x, btn_y + classic_layout::button_height - 1, btn_x + classic_layout::button_width - 1, btn_y + classic_layout::button_height - 1, sf::Color(120, 120, 150));
            }

            // Button letter with shadow
            int32_t lx = btn_x + classic_layout::button_width / 2 - 4;
            int32_t ly = btn_y + classic_layout::button_height / 2 - 6;
            rend.draw_text(button_labels[i], lx + 1, ly + 1, sf::Color(0, 0, 0), 12);
            rend.draw_text(button_labels[i], lx, ly, hovered ? sf::Color(255, 255, 200) : sf::Color(200, 200, 220), 12);
        }
    }

    // Render combat mode indicator if in combat
    if (combat_mode_) {
        uint32_t combat_frame = safe_attack_mode_ ?
            classic_sprites::combat_safe_mode : classic_sprites::combat_pk_mode;

        if (icon_spr && icon_spr->frame_count() > combat_frame) {
            icon_spr->draw(rend.window(), classic_layout::combat_x, classic_layout::combat_y, combat_frame);
        } else {
            // Fallback combat indicator
            sf::Color indicator_color = safe_attack_mode_ ?
                sf::Color(100, 200, 100) : sf::Color(200, 100, 100);
            rend.draw_rect(classic_layout::combat_x, classic_layout::combat_y, 30, 30, indicator_color, true);
            rend.draw_rect(classic_layout::combat_x, classic_layout::combat_y, 30, 30, sf::Color(150, 150, 150), false);
            const char* mode_text = safe_attack_mode_ ? "S" : "P";
            rend.draw_text(mode_text, classic_layout::combat_x + 10, classic_layout::combat_y + 8, sf::Color::White, 12);
        }
    }
}

// =============================================================================
// Hit Testing & Input
// =============================================================================

int32_t icon_panel_dialog::get_hovered_button(int32_t mouse_x, int32_t mouse_y) const {
    // Use different button positions depending on UI style
    if (ui_style_ == ui_style::classic) {
        // Classic layout button positions
        static const int32_t button_x_positions[] = {
            classic_layout::button_character_x,
            classic_layout::button_inventory_x,
            classic_layout::button_magic_x,
            classic_layout::button_skills_x,
            classic_layout::button_chat_x,
            classic_layout::button_system_x
        };

        for (int32_t i = 0; i < button_count; ++i) {
            int32_t btn_x = button_x_positions[i];
            int32_t btn_y = classic_layout::button_y;

            if (mouse_x >= btn_x && mouse_x < btn_x + classic_layout::button_width &&
                mouse_y >= btn_y && mouse_y < btn_y + classic_layout::button_height) {
                return i;
            }
        }
    } else {
        // Modern layout - buttons in a row
        if (mouse_y < panel_y || mouse_y >= panel_y + panel_height) {
            return -1;
        }

        for (int32_t i = 0; i < button_count; ++i) {
            int32_t btn_x = button_start_x + i * button_width;
            int32_t btn_y = panel_y + button_y;

            if (mouse_x >= btn_x && mouse_x < btn_x + button_width &&
                mouse_y >= btn_y && mouse_y < btn_y + button_height) {
                return i;
            }
        }
    }

    return -1;
}

bool icon_panel_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;
    if (btn != sf::Mouse::Button::Left) return false;

    int32_t clicked_button = get_hovered_button(x, y);
    if (clicked_button < 0) return false;

    // Invoke callback for clicked button
    switch (clicked_button) {
        case 0:
            if (on_character_) on_character_();
            break;
        case 1:
            if (on_inventory_) on_inventory_();
            break;
        case 2:
            if (on_spellbook_) on_spellbook_();
            break;
        case 3:
            if (on_skills_) on_skills_();
            break;
        case 4:
            if (on_chat_history_) on_chat_history_();
            break;
        case 5:
            if (on_system_menu_) on_system_menu_();
            break;
    }

    return true;
}

// =============================================================================
// Setters
// =============================================================================

void icon_panel_dialog::set_hp(int32_t current, int32_t max) {
    hp_current_ = std::max(0, current);
    hp_max_ = std::max(1, max);
}

void icon_panel_dialog::set_mp(int32_t current, int32_t max) {
    mp_current_ = std::max(0, current);
    mp_max_ = std::max(1, max);
}

void icon_panel_dialog::set_sp(int32_t current, int32_t max) {
    sp_current_ = std::max(0, current);
    sp_max_ = std::max(1, max);
}

void icon_panel_dialog::set_experience(int64_t current_exp, int64_t exp_to_level, int32_t level) {
    current_exp_ = std::max(static_cast<int64_t>(0), current_exp);
    exp_to_level_ = std::max(static_cast<int64_t>(1), exp_to_level);
    player_level_ = level;
}

void icon_panel_dialog::set_map_name(std::string_view name) {
    map_name_ = name;
}

void icon_panel_dialog::set_position(int32_t x, int32_t y) {
    player_x_ = x;
    player_y_ = y;
}

void icon_panel_dialog::set_combat_mode(bool combat) {
    combat_mode_ = combat;
}

void icon_panel_dialog::set_safe_attack_mode(bool safe) {
    safe_attack_mode_ = safe;
}

void icon_panel_dialog::set_super_attack_count(int32_t count) {
    super_attack_count_ = count;
}

void icon_panel_dialog::set_super_attack_available(bool available) {
    super_attack_available_ = available;
}

void icon_panel_dialog::set_poisoned(bool poisoned) {
    is_poisoned_ = poisoned;
}

} // namespace hb
