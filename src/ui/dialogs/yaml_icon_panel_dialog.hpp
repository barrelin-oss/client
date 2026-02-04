#pragma once

#include "ui/managed_dialog.hpp"
#include <functional>
#include <string>

namespace hb {

class sprite_manager;

// YAML-based icon panel that uses YAML for layout but custom rendering for HUD elements
// This demonstrates the hybrid approach: YAML defines positions, code handles complex rendering
class yaml_icon_panel_dialog : public managed_dialog {
public:
    explicit yaml_icon_panel_dialog(dialog_definition def);
    ~yaml_icon_panel_dialog() override = default;

    // Button click callbacks
    using button_callback = std::function<void()>;
    using sound_callback = std::function<void()>;

    void set_on_character(button_callback cb) { on_character_ = std::move(cb); }
    void set_on_inventory(button_callback cb) { on_inventory_ = std::move(cb); }
    void set_on_spellbook(button_callback cb) { on_spellbook_ = std::move(cb); }
    void set_on_skills(button_callback cb) { on_skills_ = std::move(cb); }
    void set_on_chat_history(button_callback cb) { on_chat_history_ = std::move(cb); }
    void set_on_system_menu(button_callback cb) { on_system_menu_ = std::move(cb); }
    void set_on_combat_indicator(button_callback cb) { on_combat_indicator_ = std::move(cb); }

    // Sound callback for button clicks
    void set_on_button_sound(sound_callback cb) { on_button_sound_ = std::move(cb); }

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

    // Super attack
    void set_super_attack_count(int32_t count);
    void set_super_attack_available(bool available);
    void set_alt_held(bool held);

    // Status effects
    void set_poisoned(bool poisoned);

    // Screen size (for dynamic positioning)
    void set_screen_size(uint32_t width, uint32_t height);

protected:
    // managed_dialog overrides
    void on_update_impl(float delta_time) override;
    bool on_custom_render(renderer& rend) override;
    bool on_custom_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

private:
    // Render custom HUD elements
    void render_background(renderer& rend);
    void render_hp_bar(renderer& rend, const element_def& elem);
    void render_mp_bar(renderer& rend, const element_def& elem);
    void render_sp_bar(renderer& rend, const element_def& elem);
    void render_exp_bar(renderer& rend, const element_def& elem);
    void render_location_text(renderer& rend, const element_def& elem);
    void render_combat_indicator(renderer& rend, const element_def& elem);
    void render_super_attack(renderer& rend, const element_def& elem);
    void render_buttons(renderer& rend);

    // Hit testing
    int32_t get_hovered_button(int32_t x, int32_t y) const;

    // Button callbacks
    button_callback on_character_;
    button_callback on_inventory_;
    button_callback on_spellbook_;
    button_callback on_skills_;
    button_callback on_chat_history_;
    button_callback on_system_menu_;
    button_callback on_combat_indicator_;
    sound_callback on_button_sound_;

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
    float exp_display_ = 0.0f;

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
    bool safe_attack_mode_ = false;

    // Super attack
    int32_t super_attack_count_ = 0;
    bool super_attack_available_ = false;
    bool alt_held_ = false;

    // Status effects
    bool is_poisoned_ = false;

    // UI state
    int32_t hovered_button_ = -1;
    bool mouse_in_info_area_ = false;
};

} // namespace hb
