#pragma once

#include "ui/ui_system.hpp"
#include <functional>

namespace hb
{

struct stats_component;

// Character information dialog - displays player stats
class character_dialog : public dialog
{
public:
    character_dialog();
    ~character_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;

    // Update displayed stats from component
    void update_stats(const stats_component& stats);

    // Set callbacks for stat point allocation
    using stat_callback = std::function<void(int)>; // stat index
    void set_on_add_stat(stat_callback callback) { on_add_stat_ = std::move(callback); }

    // Set available stat points
    void set_stat_points(int32_t points) { stat_points_ = points; }

private:
    void create_ui();
    void render_stat_row(renderer& rend, int32_t y, const char* name, int32_t value, int32_t stat_index);

    // Cached stat values
    int32_t strength_ = 0;
    int32_t vitality_ = 0;
    int32_t dexterity_ = 0;
    int32_t intelligence_ = 0;
    int32_t magic_ = 0;
    int32_t charisma_ = 0;

    int32_t hp_ = 0;
    int32_t max_hp_ = 0;
    int32_t mp_ = 0;
    int32_t max_mp_ = 0;
    int32_t sp_ = 0;
    int32_t max_sp_ = 0;

    uint16_t level_ = 1;
    int64_t exp_ = 0;
    int64_t exp_next_ = 100;

    int32_t attack_power_ = 0;
    int32_t magic_power_ = 0;
    int32_t defense_ = 0;
    int32_t magic_resist_ = 0;

    int32_t stat_points_ = 0;

    stat_callback on_add_stat_;

    // Stat button rects for hit testing
    static constexpr int32_t stat_button_size = 16;
};

} // namespace hb
