#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace hb
{

// Common interface for icon panel dialogs (both YAML-based and code-based)
// Eliminates dynamic_cast branching at call sites
class icon_panel_interface
{
public:
    virtual ~icon_panel_interface() = default;

    using button_callback = std::function<void()>;
    using sound_callback = std::function<void()>;

    // Button click callbacks
    virtual void set_on_character(button_callback cb) = 0;
    virtual void set_on_inventory(button_callback cb) = 0;
    virtual void set_on_spellbook(button_callback cb) = 0;
    virtual void set_on_skills(button_callback cb) = 0;
    virtual void set_on_chat_history(button_callback cb) = 0;
    virtual void set_on_system_menu(button_callback cb) = 0;
    virtual void set_on_combat_indicator(button_callback cb) = 0;
    virtual void set_on_button_sound(sound_callback cb) = 0;

    // Stats
    virtual void set_hp(int32_t current, int32_t max) = 0;
    virtual void set_mp(int32_t current, int32_t max) = 0;
    virtual void set_sp(int32_t current, int32_t max) = 0;

    // Experience
    virtual void set_experience(int64_t current_exp, int64_t exp_to_level, int32_t level) = 0;

    // Map info
    virtual void set_map_name(std::string_view name) = 0;
    virtual void set_position(int32_t x, int32_t y) = 0;

    // Combat mode
    virtual void set_combat_mode(bool combat) = 0;
    virtual void set_safe_attack_mode(bool safe) = 0;

    // Super attack
    virtual void set_super_attack_count(int32_t count) = 0;
    virtual void set_super_attack_available(bool available) = 0;
    virtual void set_alt_held(bool held) = 0;

    // Status effects
    virtual void set_poisoned(bool poisoned) = 0;

    // Screen size
    virtual void set_screen_size(uint32_t width, uint32_t height) = 0;
};

} // namespace hb
