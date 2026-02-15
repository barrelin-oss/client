#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <string>

namespace hb
{

class death_dialog : public dialog
{
public:
    death_dialog();
    ~death_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_key_press(sf::Keyboard::Key key) override;

    // Set death info
    void set_death_info(std::string_view killer_name, bool is_pvp, int32_t xp_lost);

    // Show/hide resurrection option
    void show_resurrect_option(bool show);

    // Callbacks
    using restart_callback = std::function<void()>;
    using resurrect_callback = std::function<void()>;

    void set_on_restart(restart_callback cb) { on_restart_ = std::move(cb); }
    void set_on_resurrect(resurrect_callback cb) { on_resurrect_ = std::move(cb); }

private:
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    bool is_point_in_restart_button(int32_t x, int32_t y) const;
    bool is_point_in_resurrect_button(int32_t x, int32_t y) const;

    // Death info
    std::string killer_name_;
    bool is_pvp_ = false;
    int32_t xp_lost_ = 0;

    // State
    bool resurrect_available_ = false;
    bool restart_hovered_ = false;
    bool resurrect_hovered_ = false;

    // Callbacks
    restart_callback on_restart_;
    resurrect_callback on_resurrect_;

    // Dynamic position (updated each render to stay centered)
    int32_t actual_x_ = 0;
    int32_t actual_y_ = 0;

    // Layout constants
    static constexpr int32_t dialog_width = 280;
    static constexpr int32_t dialog_height = 140;
    static constexpr int32_t top_margin = 60;

    static constexpr int32_t button_width = 100;
    static constexpr int32_t button_height = 26;
    static constexpr int32_t button_spacing = 16;
};

} // namespace hb
