#pragma once

#include "ui/managed_dialog.hpp"
#include <vector>
#include <string>
#include <optional>

namespace hb {

// Help topic structure (matches YAML format)
struct yaml_help_topic {
    std::string title;
    std::vector<std::string> content_lines;
    int32_t category = 0;
};

// Help category
struct help_category {
    std::string name;
    int32_t id = 0;
};

// YAML-based help dialog with custom rendering
// Uses managed_dialog for base functionality, adds custom rendering for:
// - Category tabs
// - Topic list with scrolling
// - Content area with text formatting
class yaml_help_dialog : public managed_dialog {
public:
    explicit yaml_help_dialog(dialog_definition def);
    ~yaml_help_dialog() override = default;

    // Load help topics from YAML data
    void set_topics(std::vector<yaml_help_topic> topics);
    void set_categories(std::vector<help_category> categories);

    // Category filter
    void set_category(int32_t category_index);
    int32_t current_category() const { return current_category_; }

    // Topic selection
    void select_topic(int32_t index);
    int32_t selected_topic() const { return selected_topic_; }

protected:
    // managed_dialog overrides for custom behavior
    void on_open_impl() override;
    void on_update_impl(float delta_time) override;
    bool on_custom_render(renderer& rend) override;
    bool on_custom_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool on_custom_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool on_custom_mouse_move(int32_t x, int32_t y) override;
    bool on_custom_mouse_wheel(int32_t x, int32_t y, int32_t delta) override;
    bool on_custom_key_press(sf::Keyboard::Key key) override;

private:
    void render_category_tabs(renderer& rend, int32_t x, int32_t y);
    void render_topic_list(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height);
    void render_content_area(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height);

    std::optional<int32_t> topic_at(int32_t mx, int32_t my) const;
    std::optional<int32_t> category_at(int32_t mx, int32_t my) const;
    std::vector<const yaml_help_topic*> get_filtered_topics() const;

    std::vector<yaml_help_topic> topics_;
    std::vector<help_category> categories_;

    int32_t selected_topic_ = -1;
    int32_t current_category_ = -1;  // -1 = all

    int32_t topic_scroll_ = 0;
    int32_t content_scroll_ = 0;

    // Configuration (can be loaded from YAML)
    int32_t visible_topics_ = 8;
    int32_t visible_content_lines_ = 12;
    int32_t topic_row_height_ = 20;
    int32_t content_line_height_ = 16;

    // Hover state
    std::optional<int32_t> hovered_topic_;
    std::optional<int32_t> hovered_category_;

    // Layout areas (computed during render)
    int32_t tab_area_x_ = 0;
    int32_t tab_area_y_ = 0;
    int32_t topic_area_x_ = 0;
    int32_t topic_area_y_ = 0;
    int32_t topic_area_w_ = 0;
    int32_t topic_area_h_ = 0;
    int32_t content_area_x_ = 0;
    int32_t content_area_y_ = 0;
    int32_t content_area_w_ = 0;
    int32_t content_area_h_ = 0;

    // Scrollbar areas (computed during render)
    struct scrollbar_info {
        int32_t track_x = 0;
        int32_t track_y = 0;
        int32_t track_w = 0;
        int32_t track_h = 0;
        int32_t thumb_y = 0;
        int32_t thumb_h = 0;
        bool visible = false;
    };
    scrollbar_info topic_scrollbar_;
    scrollbar_info content_scrollbar_;

    // Scrollbar dragging state
    enum class drag_target { none, topic_scrollbar, content_scrollbar };
    drag_target dragging_ = drag_target::none;
    int32_t drag_start_y_ = 0;
    int32_t drag_start_scroll_ = 0;

    static constexpr int32_t title_bar_height_ = 24;
    static constexpr int32_t tab_width_ = 60;
    static constexpr int32_t tab_height_ = 22;
    static constexpr int32_t scrollbar_width_ = 8;
};

} // namespace hb
