#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace hb
{

// Character information for display
struct character_display_info
{
    std::string name;
    uint16_t level = 1;
    std::string map_name;
    std::string class_name; // "Warrior" or "Mage"
    uint8_t gender = 0;     // 0 = male, 1 = female
    bool warrior = true;
    // Stats for display
    uint16_t strength = 0;
    uint16_t vitality = 0;
    uint16_t dexterity = 0;
    uint16_t intelligence = 0;
    uint16_t magic = 0;
    uint16_t charisma = 0;
};

class character_select_dialog : public dialog
{
public:
    character_select_dialog();
    ~character_select_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;

    // Character management
    void set_characters(const std::vector<character_display_info>& characters);
    void clear_characters();
    int32_t selected_index() const { return selected_index_; }
    void set_selected_index(int32_t index);

    // Callbacks
    using select_callback = std::function<void(int32_t index)>;
    using create_callback = std::function<void()>;
    using delete_callback = std::function<void(int32_t index)>;
    using enter_callback = std::function<void(int32_t index)>;

    void set_on_select(select_callback callback) { on_select_ = std::move(callback); }
    void set_on_create(create_callback callback) { on_create_ = std::move(callback); }
    void set_on_delete(delete_callback callback) { on_delete_ = std::move(callback); }
    void set_on_enter(enter_callback callback) { on_enter_ = std::move(callback); }

    // Status
    void set_status(std::string_view message, bool error = false);
    void clear_status();

    static constexpr int32_t max_characters = 4;

private:
    void create_ui();
    void update_character_display();
    void render_character_slot(renderer& rend, int32_t slot, int32_t x, int32_t y);

    std::vector<character_display_info> characters_;
    int32_t selected_index_ = -1;

    std::string status_message_;
    bool status_is_error_ = false;

    select_callback on_select_;
    create_callback on_create_;
    delete_callback on_delete_;
    enter_callback on_enter_;

    // Character slot dimensions
    static constexpr int32_t slot_width = 160;
    static constexpr int32_t slot_height = 180;
    static constexpr int32_t slot_padding = 10;
};

} // namespace hb
