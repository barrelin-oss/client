#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <cstdint>

namespace hb {

// Level up dialog - appears when player levels up to allocate stat points
class levelup_dialog : public dialog {
public:
    levelup_dialog();
    ~levelup_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;

    // Set new level and available points
    void set_level(uint16_t level);
    void set_points_available(uint16_t points);

    // Set current stats (before allocation)
    void set_stats(uint16_t str, uint16_t vit, uint16_t dex,
                   uint16_t intelligence, uint16_t mag, uint16_t cha);

    // Callbacks
    using allocate_callback = std::function<void(int stat_index)>;
    using confirm_callback = std::function<void()>;

    void set_on_allocate(allocate_callback cb) { on_allocate_ = std::move(cb); }
    void set_on_confirm(confirm_callback cb) { on_confirm_ = std::move(cb); }

    // Stat indices
    static constexpr int stat_str = 0;
    static constexpr int stat_vit = 1;
    static constexpr int stat_dex = 2;
    static constexpr int stat_int = 3;
    static constexpr int stat_mag = 4;
    static constexpr int stat_cha = 5;

private:
    void create_ui();
    void update_display();
    void add_stat(int index);

    uint16_t level_ = 1;
    uint16_t points_available_ = 0;
    uint16_t points_allocated_ = 0;

    // Current stats
    uint16_t stats_[6] = {10, 10, 10, 10, 10, 10};
    uint16_t added_[6] = {0, 0, 0, 0, 0, 0};

    allocate_callback on_allocate_;
    confirm_callback on_confirm_;

    // UI element references
    ui_label* level_label_ = nullptr;
    ui_label* points_label_ = nullptr;
    ui_label* stat_labels_[6] = {nullptr};
    ui_label* added_labels_[6] = {nullptr};
};

} // namespace hb
