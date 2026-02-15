#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <string>
#include <string_view>

namespace hb
{

class character_create_dialog : public dialog
{
public:
    character_create_dialog();
    ~character_create_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;

    // Character creation data
    struct character_data
    {
        std::string name;
        uint8_t gender = 0;     // 0 = male, 1 = female
        uint8_t skin_color = 0; // 0-2
        uint8_t hair_style = 0; // 0-7
        uint8_t hair_color = 0; // 0-3
        uint16_t strength = 10;
        uint16_t vitality = 10;
        uint16_t dexterity = 10;
        uint16_t intelligence = 10;
        uint16_t magic = 10;
        uint16_t charisma = 10;
        bool warrior = true; // true = warrior, false = mage
    };

    // Get current character data
    const character_data& get_character_data() const { return char_data_; }

    // Callbacks
    using create_callback = std::function<void(const character_data&)>;
    using cancel_callback = std::function<void()>;

    void set_on_create(create_callback callback) { on_create_ = std::move(callback); }
    void set_on_cancel(cancel_callback callback) { on_cancel_ = std::move(callback); }

    // Status
    void set_status(std::string_view message, bool error = false);
    void clear_status();

    // Reset to defaults
    void reset();

    // Starting stat points
    static constexpr uint16_t total_stat_points = 70; // Base 10 each = 60, + 10 bonus
    static constexpr uint16_t min_stat = 10;
    static constexpr uint16_t max_stat = 14; // Can add max 4 points per stat at start

private:
    void create_ui();
    void update_stat_display();
    void update_points_remaining();
    void render_character_preview(renderer& rend);
    bool validate_character();

    // Stat adjustment
    void add_stat(int stat_index);
    void remove_stat(int stat_index);
    uint16_t get_stat(int index) const;
    void set_stat(int index, uint16_t value);
    uint16_t get_stat_points_used() const;

    character_data char_data_;
    uint16_t points_remaining_ = 10; // 10 bonus points to distribute

    std::string status_message_;
    bool status_is_error_ = false;

    create_callback on_create_;
    cancel_callback on_cancel_;

    // Stat labels for updates
    ui_label* str_value_label_ = nullptr;
    ui_label* vit_value_label_ = nullptr;
    ui_label* dex_value_label_ = nullptr;
    ui_label* int_value_label_ = nullptr;
    ui_label* mag_value_label_ = nullptr;
    ui_label* cha_value_label_ = nullptr;
    ui_label* points_label_ = nullptr;
    ui_text_input* name_input_ = nullptr;
};

} // namespace hb
