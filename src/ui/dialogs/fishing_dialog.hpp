#pragma once

#include "ui/dialog_base.hpp"
#include <functional>
#include <string>
#include <cstdint>

namespace hb {

class fishing_dialog : public dialog
{
public:
    fishing_dialog();
    ~fishing_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    // Called by network handlers
    void open_fishing(std::string_view fish_name, uint8_t visual_type, int32_t catch_chance);
    void update_catch_chance(int32_t chance);
    void close_fishing();

    // Callbacks
    void set_on_catch_clicked(std::function<void()> callback) { on_catch_ = std::move(callback); }
    void set_on_cancel_clicked(std::function<void()> callback) { on_cancel_ = std::move(callback); }

private:
    std::string fish_name_;
    uint8_t visual_type_ = 0;
    int32_t catch_chance_ = 0;

    std::function<void()> on_catch_;
    std::function<void()> on_cancel_;

    int32_t hovered_element_ = -1;

    // Layout constants
    static constexpr int32_t dialog_width = 260;
    static constexpr int32_t dialog_height = 180;

    // Element indices
    static constexpr int32_t elem_catch_button = 0;
    static constexpr int32_t elem_cancel_button = 1;
};

} // namespace hb
