#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <vector>
#include <string>

namespace hb {

// Help topic structure
struct help_topic {
    std::string title;
    std::vector<std::string> content_lines;
    uint8_t category = 0;  // 0=basics, 1=combat, 2=magic, 3=items, 4=social, 5=shortcuts
};

// Help dialog - for displaying game help and tutorials
class help_dialog : public dialog {
public:
    help_dialog();
    ~help_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;
    bool handle_key_press(sf::Keyboard::Key key) override;

    // Topic management
    void add_topic(help_topic topic) { topics_.push_back(std::move(topic)); }
    void set_topics(std::vector<help_topic> topics) { topics_ = std::move(topics); }
    void clear_topics() { topics_.clear(); selected_topic_ = -1; }

    // Category filter
    void set_category(int category) { current_category_ = category; topic_scroll_ = 0; }
    int current_category() const { return current_category_; }

    // Select a topic
    void select_topic(int32_t index);
    int32_t selected_topic() const { return selected_topic_; }

    // Initialize default help content
    void load_default_help();

private:
    void render_category_tabs(renderer& rend, int32_t x, int32_t y);
    void render_topic_list(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height);
    void render_content_area(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height);
    std::optional<int32_t> topic_at(int32_t mx, int32_t my) const;
    std::optional<int32_t> category_at(int32_t mx, int32_t my) const;
    std::vector<const help_topic*> get_filtered_topics() const;

    std::vector<help_topic> topics_;
    int32_t selected_topic_ = -1;
    int current_category_ = -1;  // -1 = all

    int32_t topic_scroll_ = 0;
    int32_t content_scroll_ = 0;
    static constexpr int32_t visible_topics = 8;
    static constexpr int32_t visible_content_lines = 12;
    static constexpr int32_t topic_row_height = 20;
    static constexpr int32_t content_line_height = 16;

    std::optional<int32_t> hovered_topic_;
    std::optional<int32_t> hovered_category_;

    int32_t topic_area_x_ = 0;
    int32_t topic_area_y_ = 0;
    int32_t topic_area_w_ = 0;
    int32_t topic_area_h_ = 0;

    int32_t tab_area_x_ = 0;
    int32_t tab_area_y_ = 0;

    static constexpr int num_categories = 6;
    static constexpr const char* category_names[num_categories] = {
        "All", "Basics", "Combat", "Magic", "Items", "Keys"
    };
};

} // namespace hb
