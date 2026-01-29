#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <array>
#include <string>

namespace hb {

class sprite_manager;

// Bottom HUD panel - combines gauges, map info, and action buttons
// This is the main in-game HUD displayed at the bottom of the screen
// Supports both modern (programmatic) and classic (sprite-based) rendering
class icon_panel_dialog : public dialog {
public:
    icon_panel_dialog();
    ~icon_panel_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    // UI style control
    void set_ui_style(ui_style style) { ui_style_ = style; }
    ui_style get_ui_style() const { return ui_style_; }

    // Sprite manager for classic rendering
    void set_sprite_manager(sprite_manager* sprites) { sprites_ = sprites; }

    // Button click callbacks
    using button_callback = std::function<void()>;

    void set_on_character(button_callback cb) { on_character_ = std::move(cb); }
    void set_on_inventory(button_callback cb) { on_inventory_ = std::move(cb); }
    void set_on_spellbook(button_callback cb) { on_spellbook_ = std::move(cb); }
    void set_on_skills(button_callback cb) { on_skills_ = std::move(cb); }
    void set_on_chat_history(button_callback cb) { on_chat_history_ = std::move(cb); }
    void set_on_system_menu(button_callback cb) { on_system_menu_ = std::move(cb); }

    // Stats (HP/MP/SP)
    void set_hp(int32_t current, int32_t max);
    void set_mp(int32_t current, int32_t max);
    void set_sp(int32_t current, int32_t max);

    // Experience
    void set_experience(int64_t current_exp, int64_t exp_to_level, int32_t level);

    // Map info
    void set_map_name(std::string_view name);
    void set_position(int32_t x, int32_t y);

    // Combat mode
    void set_combat_mode(bool combat);
    void set_safe_attack_mode(bool safe);

    // Super attack (displayed when weapon skill is at 100%)
    void set_super_attack_count(int32_t count);
    void set_super_attack_available(bool available);

    // Poisoned status (affects HP bar display)
    void set_poisoned(bool poisoned);

private:
    // === Modern style rendering ===
    void render_modern(renderer& rend);
    void render_modern_gauge_bars(renderer& rend);
    void render_modern_map_info(renderer& rend);
    void render_modern_action_buttons(renderer& rend);
    void render_modern_combat_indicator(renderer& rend);
    void render_modern_super_attack_counter(renderer& rend);

    // === Classic style rendering ===
    void render_classic(renderer& rend);
    void render_classic_background(renderer& rend);
    void render_classic_gauge_bars(renderer& rend);
    void render_classic_map_info(renderer& rend);
    void render_classic_action_buttons(renderer& rend);

    // Hit testing
    int32_t get_hovered_button(int32_t mouse_x, int32_t mouse_y) const;

    // UI style (default to classic for authentic Helbreath experience)
    ui_style ui_style_ = ui_style::classic;
    sprite_manager* sprites_ = nullptr;

    // Button callbacks
    button_callback on_character_;
    button_callback on_inventory_;
    button_callback on_spellbook_;
    button_callback on_skills_;
    button_callback on_chat_history_;
    button_callback on_system_menu_;

    // Stats
    int32_t hp_current_ = 100;
    int32_t hp_max_ = 100;
    int32_t mp_current_ = 50;
    int32_t mp_max_ = 50;
    int32_t sp_current_ = 100;
    int32_t sp_max_ = 100;

    // Animated display values for smooth transitions
    float hp_display_ = 1.0f;
    float mp_display_ = 1.0f;
    float sp_display_ = 1.0f;
    float exp_display_ = 0.0f;  // Experience bar animation

    // Experience
    int64_t current_exp_ = 0;
    int64_t exp_to_level_ = 1000;
    int32_t player_level_ = 1;

    // Map info
    std::string map_name_;
    int32_t player_x_ = 0;
    int32_t player_y_ = 0;

    // Combat mode
    bool combat_mode_ = false;
    bool safe_attack_mode_ = true;

    // Super attack
    int32_t super_attack_count_ = 0;
    bool super_attack_available_ = false;

    // Status effects
    bool is_poisoned_ = false;

    // UI state
    int32_t hovered_button_ = -1;
    bool mouse_in_info_area_ = false;  // When true, show exp instead of map info

    // === Layout constants ===
    // Panel is at bottom of screen, full width
    static constexpr int32_t panel_height = 46;
    static constexpr int32_t panel_y = 434;  // 480 - 46

    // HP/MP bars (left section)
    static constexpr int32_t hp_bar_x = 23;
    static constexpr int32_t hp_bar_y = 3;   // Relative to panel
    static constexpr int32_t mp_bar_y = 25;  // Relative to panel
    static constexpr int32_t hp_mp_bar_width = 101;
    static constexpr int32_t hp_mp_bar_height = 18;

    // SP bar (middle-left section) - "Stamina" bar
    static constexpr int32_t sp_bar_x = 147;
    static constexpr int32_t sp_bar_y = 1;   // Relative to panel
    static constexpr int32_t sp_bar_width = 167;
    static constexpr int32_t sp_bar_height = 12;

    // EXP bar (below SP bar)
    static constexpr int32_t exp_bar_x = 147;
    static constexpr int32_t exp_bar_y = 33;  // Relative to panel
    static constexpr int32_t exp_bar_width = 167;
    static constexpr int32_t exp_bar_height = 10;

    // Info area (center section)
    static constexpr int32_t info_area_x = 140;
    static constexpr int32_t info_area_y = 16;
    static constexpr int32_t info_area_width = 183;
    static constexpr int32_t info_area_height = 16;

    // Action buttons (right section)
    static constexpr int32_t button_start_x = 412;
    static constexpr int32_t button_y = 0;    // Relative to panel
    static constexpr int32_t button_width = 37;
    static constexpr int32_t button_height = 41;
    static constexpr int32_t button_count = 6;

    // Combat indicator position
    static constexpr int32_t combat_x = 368;
    static constexpr int32_t combat_y = 6;

    // Super attack counter position
    static constexpr int32_t super_attack_x = 362;
    static constexpr int32_t super_attack_y = 0;

    // === Classic UI sprite indices ===
    // From GameDialog.pak, sprite index 6 (DEF_SPRID_INTERFACE_ND_ICONPANNEL = 64)
    // These are frame indices within the icon panel sprite
    struct classic_sprites {
        // PAK file and sprite index
        static constexpr const char* pak_name = "GameDialog";
        static constexpr uint32_t sprite_index = 6;  // Index in PAK file

        // Frame indices within the sprite
        static constexpr uint32_t panel_background = 14;     // Main panel background
        static constexpr uint32_t panel_hover_highlight = 16; // Hover highlight effect

        // HP/MP/SP bar fills (used with variable width rendering)
        static constexpr uint32_t hp_mp_bar_fill = 12;       // HP and MP bar fill
        static constexpr uint32_t sp_bar_fill = 13;          // SP bar fill

        // Combat mode indicators (position: 368, 440)
        static constexpr uint32_t combat_safe_mode = 4;      // Safe attack mode (green)
        static constexpr uint32_t combat_pk_mode = 5;        // PK mode (red)
        static constexpr uint32_t combat_animation = 3;      // Combat animation overlay

        // Crusade/War icons (position: 322, 434)
        static constexpr uint32_t crusade_slayer = 0;        // Slayer faction
        static constexpr uint32_t crusade_slayer_hover = 1;  // Slayer hover
        static constexpr uint32_t crusade_slayer_alt = 2;    // Alternative state
        static constexpr uint32_t crusade_icon = 15;         // Crusade icon

        // Action buttons (frames 6-11, Y: 434)
        static constexpr uint32_t button_character = 6;      // X: 412 - Character (F5)
        static constexpr uint32_t button_inventory = 7;      // X: 449 - Inventory (F6)
        static constexpr uint32_t button_magic = 8;          // X: 486 - Magic/Spellbook (F7)
        static constexpr uint32_t button_skills = 9;         // X: 523 - Skills (F8)
        static constexpr uint32_t button_chat = 10;          // X: 560 - Chat History (F9)
        static constexpr uint32_t button_system = 11;        // X: 597 - System Menu (F12)
    };

    // Classic UI layout positions (from legacy code)
    struct classic_layout {
        // Bar positions
        static constexpr int32_t hp_bar_x = 23;
        static constexpr int32_t hp_bar_y = 437;
        static constexpr int32_t mp_bar_y = 459;
        static constexpr int32_t sp_bar_x = 147;
        static constexpr int32_t sp_bar_y = 435;
        static constexpr int32_t bar_max_width = 101;  // HP/MP bar width
        static constexpr int32_t sp_bar_max_width = 167;

        // Combat indicator
        static constexpr int32_t combat_x = 368;
        static constexpr int32_t combat_y = 440;

        // Button positions (Y: 434 for all, X varies)
        static constexpr int32_t button_y = 434;
        static constexpr int32_t button_character_x = 412;
        static constexpr int32_t button_inventory_x = 449;
        static constexpr int32_t button_magic_x = 486;
        static constexpr int32_t button_skills_x = 523;
        static constexpr int32_t button_chat_x = 560;
        static constexpr int32_t button_system_x = 597;
        static constexpr int32_t button_width = 35;
        static constexpr int32_t button_height = 41;
    };
};

} // namespace hb
