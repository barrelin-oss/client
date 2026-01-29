#pragma once

#include "ui/screens/screen_base.hpp"
#include "graphics/menu_character_renderer.hpp"
#include <functional>
#include <string>

namespace hb {

class menu_character_renderer;

// Sprite IDs for character create screen (from SpriteID.h)
namespace charcreate_sprites {
    inline constexpr uint16_t new_char = 58;      // DEF_SPRID_INTERFACE_ND_NEWCHAR (GameDialog sprite 9)
    inline constexpr uint16_t button = 71;        // DEF_SPRID_INTERFACE_ND_BUTTON (DialogText sprite 1)
    inline constexpr uint16_t mouse_cursor = 0;   // DEF_SPRID_MOUSECURSOR
}

// Character creation data to send to server
struct character_create_data {
    std::string name;
    uint8_t gender = 1;       // 1 = male, 2 = female
    uint8_t skin_color = 1;   // 1-3
    uint8_t hair_style = 0;   // 0-7
    uint8_t hair_color = 0;   // 0-15
    uint8_t underwear_color = 0; // 0-7
    uint8_t strength = 10;
    uint8_t vitality = 10;
    uint8_t dexterity = 10;
    uint8_t intelligence = 10;
    uint8_t magic = 10;
    uint8_t charisma = 10;
};

// Character create screen - modern equivalent of UpdateScreen_OnCreateNewCharacter
class character_create_screen : public screen_base {
public:
    using create_callback = std::function<void(const character_create_data& data)>;
    using cancel_callback = std::function<void()>;

    character_create_screen() = default;
    ~character_create_screen() override = default;

    void on_enter() override;
    void on_exit() override;
    bool update(float delta_time, const input& inp) override;
    void render(renderer& rend, sprite_manager& sprites) override;

    // Set callbacks
    void set_on_create(create_callback callback) { on_create_ = std::move(callback); }
    void set_on_cancel(cancel_callback callback) { on_cancel_ = std::move(callback); }

    // Set character renderer for drawing character preview
    void set_character_renderer(menu_character_renderer* renderer) { char_renderer_ = renderer; }

private:
    void draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y);
    void handle_text_input(const input& inp);
    void try_create();
    void apply_warrior_preset();
    void apply_mage_preset();
    void apply_merchant_preset();
    int32_t remaining_points() const;
    bool is_valid_character() const;

    // Callbacks
    create_callback on_create_;
    cancel_callback on_cancel_;

    // Character data
    std::string char_name_;
    uint8_t gender_ = 1;
    uint8_t skin_color_ = 1;
    uint8_t hair_style_ = 0;
    uint8_t hair_color_ = 0;
    uint8_t underwear_color_ = 0;

    // Stats (10-14 range, total 70 points)
    uint8_t str_ = 10;
    uint8_t vit_ = 10;
    uint8_t dex_ = 10;
    uint8_t int_ = 10;
    uint8_t mag_ = 10;
    uint8_t chr_ = 10;

    // UI state
    static constexpr int32_t name_max_length = 11;
    static constexpr int32_t total_stat_points = 70;
    static constexpr int32_t min_stat = 10;
    static constexpr int32_t max_stat = 14;

    // Button IDs
    static constexpr int32_t btn_name_field = 1;
    static constexpr int32_t btn_gender_left = 2;
    static constexpr int32_t btn_gender_right = 3;
    static constexpr int32_t btn_skin_left = 4;
    static constexpr int32_t btn_skin_right = 5;
    static constexpr int32_t btn_hair_style_left = 6;
    static constexpr int32_t btn_hair_style_right = 7;
    static constexpr int32_t btn_hair_color_left = 8;
    static constexpr int32_t btn_hair_color_right = 9;
    static constexpr int32_t btn_underwear_left = 10;
    static constexpr int32_t btn_underwear_right = 11;
    static constexpr int32_t btn_str_up = 12;
    static constexpr int32_t btn_str_down = 13;
    static constexpr int32_t btn_vit_up = 14;
    static constexpr int32_t btn_vit_down = 15;
    static constexpr int32_t btn_dex_up = 16;
    static constexpr int32_t btn_dex_down = 17;
    static constexpr int32_t btn_int_up = 18;
    static constexpr int32_t btn_int_down = 19;
    static constexpr int32_t btn_mag_up = 20;
    static constexpr int32_t btn_mag_down = 21;
    static constexpr int32_t btn_chr_up = 22;
    static constexpr int32_t btn_chr_down = 23;
    static constexpr int32_t btn_create = 24;
    static constexpr int32_t btn_cancel = 25;
    static constexpr int32_t btn_warrior = 26;
    static constexpr int32_t btn_mage = 27;
    static constexpr int32_t btn_merchant = 28;

    // Animation state (for character preview)
    int32_t menu_frame_ = 0;
    int32_t menu_dir_ = 1;
    int32_t menu_dir_count_ = 0;
    float frame_timer_ = 0.0f;

    // Cursor blink state
    float cursor_timer_ = 0.0f;
    bool cursor_visible_ = true;

    // Character renderer (borrowed pointer)
    menu_character_renderer* char_renderer_ = nullptr;
};

} // namespace hb
