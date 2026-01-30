#pragma once

#include "ui/screens/screen_base.hpp"
#include "graphics/menu_character_renderer.hpp"
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace hb {

class menu_character_renderer;

// Sprite IDs for character select screen (from SpriteID.h)
namespace charselect_sprites {
    inline constexpr uint16_t select_char = 57;     // DEF_SPRID_INTERFACE_ND_SELECTCHAR (GameDialog sprite 8)
    inline constexpr uint16_t button = 71;          // DEF_SPRID_INTERFACE_ND_BUTTON (DialogText sprite 1)
    inline constexpr uint16_t mouse_cursor = 0;     // DEF_SPRID_MOUSECURSOR
}

// Character slot display info
struct char_slot_info {
    bool has_character = false;
    std::string name;
    int32_t level = 0;
    int32_t exp = 0;
    std::string class_name;  // "Warrior" or "Mage"

    // Appearance data for rendering
    uint8_t gender = 1;           // 1 = male, 2 = female
    uint8_t skin_color = 1;       // 1-3
    uint8_t hair_style = 0;       // 0-7
    uint8_t hair_color = 0;       // 0-15
    uint8_t underwear_color = 0;  // 0-7

    // Equipment (0 = not equipped)
    uint8_t body_armor = 0;
    uint8_t arm_armor = 0;
    uint8_t pants = 0;
    uint8_t boots = 0;
    uint8_t helmet = 0;
    uint8_t mantle = 0;
    uint8_t weapon = 0;
    uint8_t shield = 0;
};

// Character select screen - modern equivalent of UpdateScreen_OnSelectCharacter
class character_select_screen : public screen_base {
public:
    using select_callback = std::function<void(int32_t index)>;
    using create_callback = std::function<void()>;
    using delete_callback = std::function<void(int32_t index)>;
    using logout_callback = std::function<void()>;

    character_select_screen() = default;
    ~character_select_screen() override = default;

    void on_enter() override;
    void on_exit() override;
    bool update(float delta_time, const input& inp) override;
    void render(renderer& rend, sprite_manager& sprites) override;
    void render_cursor(renderer& rend, sprite_manager& sprites) override;

    // Set character data
    void set_characters(const std::vector<char_slot_info>& characters);

    // Set character renderer for drawing character previews
    void set_character_renderer(menu_character_renderer* renderer) { char_renderer_ = renderer; }

    // Set callbacks
    void set_on_select(select_callback callback) { on_select_ = std::move(callback); }
    void set_on_create(create_callback callback) { on_create_ = std::move(callback); }
    void set_on_delete(delete_callback callback) { on_delete_ = std::move(callback); }
    void set_on_logout(logout_callback callback) { on_logout_ = std::move(callback); }

    // Get selected character
    int32_t selected_index() const { return current_focus_ - 1; }

private:
    void draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y);
    void enter_game();
    void create_character();
    void delete_character();
    void logout();

    // Callbacks
    select_callback on_select_;
    create_callback on_create_;
    delete_callback on_delete_;
    logout_callback on_logout_;

    // Character slots (up to 4)
    std::array<char_slot_info, 4> characters_{};
    int32_t total_characters_ = 0;

    // UI button indices
    static constexpr int32_t btn_char1 = 1;
    static constexpr int32_t btn_char2 = 2;
    static constexpr int32_t btn_char3 = 3;
    static constexpr int32_t btn_char4 = 4;
    static constexpr int32_t btn_enter_game = 5;
    static constexpr int32_t btn_new_char = 6;
    static constexpr int32_t btn_delete_char = 7;
    static constexpr int32_t btn_change_password = 8;  // No action implemented
    static constexpr int32_t btn_logout = 9;

    // Animation state (for character preview)
    int32_t menu_frame_ = 0;
    int32_t menu_dir_ = 1;
    int32_t menu_dir_count_ = 0;
    float frame_timer_ = 0.0f;

    // Character renderer (borrowed pointer)
    menu_character_renderer* char_renderer_ = nullptr;
};

} // namespace hb
