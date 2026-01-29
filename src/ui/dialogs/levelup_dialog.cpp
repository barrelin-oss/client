#include "ui/dialogs/levelup_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>

namespace hb {

levelup_dialog::levelup_dialog()
    : dialog(dialog_type::none) {  // Custom dialog type
    set_title("Level Up!");
    set_bounds({
        static_cast<int32_t>(screen_width) / 2 - 150,
        static_cast<int32_t>(screen_height) / 2 - 150,
        300, 300
    });
    set_closeable(false);
    set_draggable(true);
    set_modal(true);

    create_ui();
}

void levelup_dialog::create_ui() {
    int32_t y = 45;

    // Level display
    auto level_title = std::make_unique<ui_label>();
    level_title->set_bounds({20, y, 260, 24});
    level_title->set_text("Congratulations!");
    level_title->set_text_color(sf::Color(255, 220, 100));
    level_title->set_alignment(ui_label::alignment::center);
    add_child(std::move(level_title));

    y += 28;

    auto level_label = std::make_unique<ui_label>();
    level_label->set_id("level_label");
    level_label->set_bounds({20, y, 260, 20});
    level_label->set_text("You reached Level 1");
    level_label->set_alignment(ui_label::alignment::center);
    level_label_ = level_label.get();
    add_child(std::move(level_label));

    y += 30;

    // Points available
    auto points_label = std::make_unique<ui_label>();
    points_label->set_id("points_label");
    points_label->set_bounds({20, y, 260, 20});
    points_label->set_text("Points: 0");
    points_label->set_text_color(sf::Color(100, 255, 100));
    points_label->set_alignment(ui_label::alignment::center);
    points_label_ = points_label.get();
    add_child(std::move(points_label));

    y += 35;

    // Stat rows
    static const char* stat_names[] = {"STR", "VIT", "DEX", "INT", "MAG", "CHA"};

    for (int i = 0; i < 6; ++i) {
        // Stat name
        auto name_label = std::make_unique<ui_label>();
        name_label->set_bounds({30, y, 40, 20});
        name_label->set_text(stat_names[i]);
        add_child(std::move(name_label));

        // Current value
        auto value_label = std::make_unique<ui_label>();
        value_label->set_bounds({80, y, 40, 20});
        value_label->set_text("10");
        value_label->set_alignment(ui_label::alignment::right);
        stat_labels_[i] = value_label.get();
        add_child(std::move(value_label));

        // Added points indicator
        auto added_label = std::make_unique<ui_label>();
        added_label->set_bounds({125, y, 40, 20});
        added_label->set_text("");
        added_label->set_text_color(sf::Color(100, 255, 100));
        added_labels_[i] = added_label.get();
        add_child(std::move(added_label));

        // Add button
        auto add_btn = std::make_unique<ui_button>();
        add_btn->set_bounds({180, y - 2, 30, 24});
        add_btn->set_text("+");
        add_btn->set_on_click([this, i]() { add_stat(i); });
        add_child(std::move(add_btn));

        y += 28;
    }

    y += 10;

    // Confirm button
    auto confirm_btn = std::make_unique<ui_button>();
    confirm_btn->set_id("confirm_btn");
    confirm_btn->set_bounds({100, y, 100, 32});
    confirm_btn->set_text("Confirm");
    confirm_btn->set_on_click([this]() {
        if (points_available_ == 0 && on_confirm_) {
            on_confirm_();
            close();
        }
    });
    add_child(std::move(confirm_btn));
}

void levelup_dialog::update(float delta_time, const input& inp) {
    if (!visible_) return;
    dialog::update(delta_time, inp);
}

void levelup_dialog::render(renderer& rend) {
    if (!visible_) return;
    dialog::render(rend);

    // Add some visual flair for level up
    // Could add particle effects, glow, etc.
}

void levelup_dialog::set_level(uint16_t level) {
    level_ = level;
    if (level_label_) {
        level_label_->set_text("You reached Level " + std::to_string(level));
    }
}

void levelup_dialog::set_points_available(uint16_t points) {
    points_available_ = points;
    points_allocated_ = 0;

    // Reset added points
    for (int i = 0; i < 6; ++i) {
        added_[i] = 0;
    }

    update_display();
}

void levelup_dialog::set_stats(uint16_t str, uint16_t vit, uint16_t dex,
                                uint16_t intelligence, uint16_t mag, uint16_t cha) {
    stats_[0] = str;
    stats_[1] = vit;
    stats_[2] = dex;
    stats_[3] = intelligence;
    stats_[4] = mag;
    stats_[5] = cha;

    update_display();
}

void levelup_dialog::update_display() {
    // Update points label
    if (points_label_) {
        uint16_t remaining = points_available_ - points_allocated_;
        points_label_->set_text("Points: " + std::to_string(remaining));
        points_label_->set_text_color(
            remaining > 0 ? sf::Color(100, 255, 100) : sf::Color(200, 200, 200)
        );
    }

    // Update stat displays
    for (int i = 0; i < 6; ++i) {
        if (stat_labels_[i]) {
            stat_labels_[i]->set_text(std::to_string(stats_[i] + added_[i]));
        }
        if (added_labels_[i]) {
            if (added_[i] > 0) {
                added_labels_[i]->set_text("+" + std::to_string(added_[i]));
            } else {
                added_labels_[i]->set_text("");
            }
        }
    }
}

void levelup_dialog::add_stat(int index) {
    if (index < 0 || index >= 6) return;

    uint16_t remaining = points_available_ - points_allocated_;
    if (remaining == 0) {
        return;
    }

    added_[index]++;
    points_allocated_++;

    update_display();

    // Call callback for network sync
    if (on_allocate_) {
        on_allocate_(index);
    }

    spdlog::debug("Level up: added point to stat {}, remaining: {}",
        index, points_available_ - points_allocated_);
}

} // namespace hb
