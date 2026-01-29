#pragma once

#include "ui/ui_system.hpp"
#include <cstdint>

namespace hb {

// Gauge panel - HP/MP/SP/EXP bars displayed during gameplay
class gauge_panel_dialog : public dialog {
public:
    gauge_panel_dialog();
    ~gauge_panel_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;

    // Set values
    void set_hp(int32_t current, int32_t max);
    void set_mp(int32_t current, int32_t max);
    void set_sp(int32_t current, int32_t max);
    void set_exp(int64_t current, int64_t next_level);

    // Get values
    int32_t current_hp() const { return current_hp_; }
    int32_t max_hp() const { return max_hp_; }
    int32_t current_mp() const { return current_mp_; }
    int32_t max_mp() const { return max_mp_; }
    int32_t current_sp() const { return current_sp_; }
    int32_t max_sp() const { return max_sp_; }

    // Player info
    void set_player_name(std::string_view name);
    void set_player_level(uint16_t level);
    void set_player_class(bool warrior);

    // Show/hide numeric values
    void set_show_values(bool show) { show_values_ = show; }
    bool show_values() const { return show_values_; }

private:
    void render_bar(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height,
                   float percent, sf::Color fill_color, sf::Color bg_color,
                   std::string_view label, std::string_view value_text);

    // Current values
    int32_t current_hp_ = 100;
    int32_t max_hp_ = 100;
    int32_t current_mp_ = 50;
    int32_t max_mp_ = 50;
    int32_t current_sp_ = 100;
    int32_t max_sp_ = 100;
    int64_t current_exp_ = 0;
    int64_t exp_to_next_level_ = 1000;

    // Player info
    std::string player_name_;
    uint16_t player_level_ = 1;
    bool is_warrior_ = true;

    // Display options
    bool show_values_ = true;

    // Animation
    float hp_display_ = 1.0f;  // Smoothly interpolated display values
    float mp_display_ = 1.0f;
    float sp_display_ = 1.0f;
    float exp_display_ = 0.0f;

    static constexpr int32_t bar_width = 200;
    static constexpr int32_t bar_height = 16;
    static constexpr int32_t bar_spacing = 4;
};

} // namespace hb
