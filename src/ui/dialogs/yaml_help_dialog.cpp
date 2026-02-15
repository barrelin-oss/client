#include "ui/dialogs/yaml_help_dialog.hpp"
#include "graphics/renderer.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <format>
#include <cstring>

namespace hb
{

yaml_help_dialog::yaml_help_dialog(dialog_definition def) : managed_dialog(std::move(def)) {}

void yaml_help_dialog::set_topics(std::vector<yaml_help_topic> topics)
{
    topics_ = std::move(topics);
    selected_topic_ = -1;
    topic_scroll_ = 0;
    content_scroll_ = 0;
}

void yaml_help_dialog::set_categories(std::vector<help_category> categories)
{
    categories_ = std::move(categories);
}

void yaml_help_dialog::set_category(int32_t category_index)
{
    if (category_index < 0 || category_index >= static_cast<int32_t>(categories_.size()))
    {
        current_category_ = -1;
    }
    else
    {
        current_category_ = categories_[category_index].id;
    }
    topic_scroll_ = 0;
    selected_topic_ = -1;
    content_scroll_ = 0;
}

void yaml_help_dialog::select_topic(int32_t index)
{
    auto filtered = get_filtered_topics();
    if (index >= 0 && index < static_cast<int32_t>(filtered.size()))
    {
        selected_topic_ = index;
        content_scroll_ = 0;
    }
}

void yaml_help_dialog::on_open_impl()
{
    selected_topic_ = -1;
    topic_scroll_ = 0;
    content_scroll_ = 0;
}

void yaml_help_dialog::on_update_impl(float /*delta_time*/)
{
    // No per-frame updates needed
}

bool yaml_help_dialog::on_custom_render(renderer& rend)
{
    const auto& def = definition();
    auto dlg_bounds = bounds();

    // Draw background
    sf::Color bg_color = def.background_color.value_or(sf::Color(32, 32, 45, 245));
    rend.draw_rect(dlg_bounds.x, dlg_bounds.y, dlg_bounds.width, dlg_bounds.height, bg_color, true);

    // Draw border
    sf::Color border_color = def.border_color.value_or(sf::Color(70, 70, 90));
    rend.draw_rect(dlg_bounds.x, dlg_bounds.y, dlg_bounds.width, dlg_bounds.height, border_color, false);

    // Draw title bar
    sf::Color title_bar_color = def.title_bar_color.value_or(sf::Color(50, 50, 70));
    rend.draw_rect(dlg_bounds.x, dlg_bounds.y, dlg_bounds.width, title_bar_height_, title_bar_color, true);
    rend.draw_line(dlg_bounds.x,
                   dlg_bounds.y + title_bar_height_,
                   dlg_bounds.x + dlg_bounds.width,
                   dlg_bounds.y + title_bar_height_,
                   sf::Color(100, 100, 140));

    // Title text
    rend.draw_text(def.title, dlg_bounds.x + 8, dlg_bounds.y + 4, sf::Color::White);

    // Close button
    if (def.closeable)
    {
        int32_t close_x = dlg_bounds.x + dlg_bounds.width - 20;
        int32_t close_y = dlg_bounds.y + 4;
        rend.draw_rect(close_x, close_y, 16, 16, sf::Color(120, 60, 60), true);
        rend.draw_text("X", close_x + 4, close_y + 1, sf::Color::White);
    }

    // Get element positions from YAML definition (with fallbacks)
    // Category tabs area
    const element_def* tabs_elem = def.find_element("category_tabs_area");
    int32_t tabs_x = dlg_bounds.x + (tabs_elem ? tabs_elem->bounds.x : 10);
    int32_t tabs_y = dlg_bounds.y + title_bar_height_ + (tabs_elem ? tabs_elem->bounds.y : 8);
    int32_t tabs_w = tabs_elem ? tabs_elem->bounds.width : (dlg_bounds.width - 20);

    // Topic list area
    const element_def* topic_elem = def.find_element("topic_list_area");
    int32_t topic_x = dlg_bounds.x + (topic_elem ? topic_elem->bounds.x : 10);
    int32_t topic_y = dlg_bounds.y + title_bar_height_ + (topic_elem ? topic_elem->bounds.y : 40);
    int32_t topic_w = topic_elem ? topic_elem->bounds.width : 140;
    int32_t topic_h = topic_elem ? topic_elem->bounds.height : (visible_topics_ * topic_row_height_);

    // Content area
    const element_def* content_elem = def.find_element("content_area");
    int32_t content_x = dlg_bounds.x + (content_elem ? content_elem->bounds.x : (topic_w + 20));
    int32_t content_y = dlg_bounds.y + title_bar_height_ + (content_elem ? content_elem->bounds.y : 40);
    int32_t content_w = content_elem ? content_elem->bounds.width : (dlg_bounds.width - topic_w - 30);
    int32_t content_h = content_elem ? content_elem->bounds.height : (visible_content_lines_ * content_line_height_);

    // Scroll hint label
    const element_def* hint_elem = def.find_element("scroll_hint");
    int32_t hint_x = dlg_bounds.x + (hint_elem ? hint_elem->bounds.x : content_x - dlg_bounds.x);
    int32_t hint_y = dlg_bounds.y + title_bar_height_ +
                     (hint_elem ? hint_elem->bounds.y : (content_y - dlg_bounds.y - title_bar_height_ + content_h + 8));

    // Update visible counts based on area heights
    visible_topics_ = topic_h / topic_row_height_;
    visible_content_lines_ = content_h / content_line_height_;

    // Store tab area for hit testing
    tab_area_x_ = tabs_x;
    tab_area_y_ = tabs_y;

    // Render category tabs
    render_category_tabs(rend, tabs_x, tabs_y);

    // Separator line below tabs
    int32_t sep_y = tabs_y + tab_height_ + 4;
    rend.draw_line(tabs_x, sep_y, tabs_x + tabs_w, sep_y, sf::Color(80, 80, 100));

    // Store topic area for hit testing
    topic_area_x_ = topic_x;
    topic_area_y_ = topic_y;
    topic_area_w_ = topic_w;
    topic_area_h_ = topic_h;

    // Store content area for hit testing
    content_area_x_ = content_x;
    content_area_y_ = content_y;
    content_area_w_ = content_w;
    content_area_h_ = content_h;

    // Render topic list
    render_topic_list(rend, topic_x, topic_y, topic_w, topic_h);

    // Render content area
    render_content_area(rend, content_x, content_y, content_w, content_h);

    // Scroll hint for content
    if (selected_topic_ >= 0)
    {
        auto filtered = get_filtered_topics();
        if (selected_topic_ < static_cast<int32_t>(filtered.size()))
        {
            const auto* topic = filtered[selected_topic_];
            if (topic->content_lines.size() > static_cast<size_t>(visible_content_lines_))
            {
                std::string hint =
                    std::format("Use Up/Down to scroll ({}/{})",
                                content_scroll_ + 1,
                                static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines_ + 1);
                sf::Color hint_color = hint_elem && hint_elem->text_color.has_value() ? hint_elem->text_color.value()
                                                                                      : sf::Color(150, 150, 150);
                rend.draw_text(hint, hint_x, hint_y, hint_color, 10);
            }
        }
    }

    return true; // We handled all rendering
}

void yaml_help_dialog::render_category_tabs(renderer& rend, int32_t x, int32_t y)
{
    for (size_t i = 0; i < categories_.size(); ++i)
    {
        int32_t tab_x = x + static_cast<int32_t>(i) * (tab_width_ + 4);
        int32_t actual_category = categories_[i].id;

        bool selected = current_category_ == actual_category;
        bool hovered = hovered_category_.has_value() && hovered_category_.value() == static_cast<int32_t>(i);

        sf::Color bg_color;
        if (selected)
        {
            bg_color = sf::Color(60, 80, 100);
        }
        else if (hovered)
        {
            bg_color = sf::Color(50, 50, 65);
        }
        else
        {
            bg_color = sf::Color(40, 40, 50);
        }

        rend.draw_rect(tab_x, y, tab_width_, tab_height_, bg_color, true);
        if (selected)
        {
            rend.draw_rect(tab_x, y, tab_width_, tab_height_, sf::Color(100, 120, 150), false);
        }

        sf::Color text_color = selected ? sf::Color::White : sf::Color(180, 180, 180);
        int32_t text_x = tab_x + (tab_width_ - static_cast<int32_t>(categories_[i].name.length() * 5)) / 2;
        rend.draw_text(categories_[i].name, text_x, y + 5, text_color, 10);
    }
}

void yaml_help_dialog::render_topic_list(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height)
{
    // Background
    rend.draw_rect(x, y, width, height, sf::Color(30, 30, 40), true);
    rend.draw_rect(x, y, width, height, sf::Color(60, 60, 80), false);

    auto filtered = get_filtered_topics();

    for (int32_t i = 0; i < visible_topics_; ++i)
    {
        int32_t topic_idx = topic_scroll_ + i;
        if (topic_idx >= static_cast<int32_t>(filtered.size()))
            break;

        const auto* topic = filtered[topic_idx];
        int32_t row_y = y + i * topic_row_height_;

        bool hovered = hovered_topic_.has_value() && hovered_topic_.value() == topic_idx;
        bool selected = selected_topic_ == topic_idx;

        sf::Color bg_color;
        if (selected)
        {
            bg_color = sf::Color(50, 70, 90);
        }
        else if (hovered)
        {
            bg_color = sf::Color(40, 45, 55);
        }
        else
        {
            bg_color = sf::Color(35, 35, 45);
        }

        rend.draw_rect(x + 2, row_y + 1, width - 4, topic_row_height_ - 2, bg_color, true);

        sf::Color text_color = selected ? sf::Color::White : sf::Color(200, 200, 200);
        std::string title = topic->title;
        if (title.length() > 18)
        {
            title = title.substr(0, 15) + "...";
        }
        rend.draw_text(title, x + 6, row_y + 4, text_color, 11);
    }

    // Scroll indicator
    if (filtered.size() > static_cast<size_t>(visible_topics_))
    {
        int32_t scroll_x = x + width - scrollbar_width_;
        int32_t scroll_y = y + 2;
        int32_t scroll_height = height - 4;

        float visible_ratio = static_cast<float>(visible_topics_) / static_cast<float>(filtered.size());
        int32_t thumb_height = std::max(15, static_cast<int32_t>(scroll_height * visible_ratio));

        float scroll_ratio = static_cast<float>(topic_scroll_) / static_cast<float>(filtered.size() - visible_topics_);
        int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_height - thumb_height) * scroll_ratio);

        // Store scrollbar info for hit testing
        topic_scrollbar_.track_x = scroll_x;
        topic_scrollbar_.track_y = scroll_y;
        topic_scrollbar_.track_w = scrollbar_width_;
        topic_scrollbar_.track_h = scroll_height;
        topic_scrollbar_.thumb_y = thumb_y;
        topic_scrollbar_.thumb_h = thumb_height;
        topic_scrollbar_.visible = true;

        // Draw track
        rend.draw_rect(scroll_x, scroll_y, scrollbar_width_, scroll_height, sf::Color(25, 25, 35), true);

        // Draw thumb (highlight if dragging)
        sf::Color thumb_color =
            (dragging_ == drag_target::topic_scrollbar) ? sf::Color(100, 100, 130) : sf::Color(70, 70, 90);
        rend.draw_rect(scroll_x, thumb_y, scrollbar_width_, thumb_height, thumb_color, true);
    }
    else
    {
        topic_scrollbar_.visible = false;
    }
}

void yaml_help_dialog::render_content_area(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height)
{
    // Background
    rend.draw_rect(x, y, width, height, sf::Color(25, 28, 35), true);
    rend.draw_rect(x, y, width, height, sf::Color(60, 60, 80), false);

    auto filtered = get_filtered_topics();
    if (selected_topic_ < 0 || selected_topic_ >= static_cast<int32_t>(filtered.size()))
    {
        rend.draw_text("Select a topic from the list.", x + 10, y + 20, sf::Color(150, 150, 150));
        return;
    }

    const auto* topic = filtered[selected_topic_];

    // Title
    rend.draw_text(topic->title, x + 8, y + 6, sf::Color(200, 200, 255));
    rend.draw_line(x + 4, y + 22, x + width - 4, y + 22, sf::Color(50, 50, 70));

    // Content
    int32_t line_y = y + 28;
    int32_t end_line =
        std::min(content_scroll_ + visible_content_lines_, static_cast<int32_t>(topic->content_lines.size()));

    for (int32_t i = content_scroll_; i < end_line; ++i)
    {
        const auto& line = topic->content_lines[i];

        if (line.empty())
        {
            // Empty line - just advance
            line_y += content_line_height_;
            continue;
        }

        // Text formatting based on first character prefix
        // The prefix character is included in display except for # (headers)
        char prefix = line[0];
        sf::Color text_color;
        std::string display_text;
        int32_t indent = 8;

        switch (prefix)
        {
        case '#':
            // Header - gold/tan, prefix stripped
            text_color = sf::Color(200, 180, 140);
            display_text = line.substr(1);
            break;

        case '*':
            // Bullet point - green
            text_color = sf::Color(180, 200, 180);
            display_text = line;
            break;

        case '-':
            // Sub-bullet/secondary - dimmer gray, indented
            text_color = sf::Color(170, 170, 180);
            display_text = line;
            indent = 16;
            break;

        case '!':
            // Warning/important - red
            text_color = sf::Color(220, 140, 140);
            display_text = line.substr(1); // Strip prefix
            break;

        case '>':
            // Tip/info - blue
            text_color = sf::Color(140, 180, 220);
            display_text = line.substr(1); // Strip prefix
            break;

        case '@':
            // Command/shortcut - cyan
            text_color = sf::Color(140, 220, 220);
            display_text = line.substr(1); // Strip prefix
            break;

        case '~':
            // Muted/note - dim gray
            text_color = sf::Color(140, 140, 150);
            display_text = line.substr(1); // Strip prefix
            break;

        case '+':
            // Positive/success - bright green
            text_color = sf::Color(140, 220, 140);
            display_text = line.substr(1); // Strip prefix
            break;

        default:
            // Normal text - light gray
            text_color = sf::Color(220, 220, 220);
            display_text = line;
            break;
        }

        rend.draw_text(display_text, x + indent, line_y, text_color, 11);
        line_y += content_line_height_;
    }

    // Content scrollbar
    int32_t total_lines = static_cast<int32_t>(topic->content_lines.size());
    if (total_lines > visible_content_lines_)
    {
        int32_t scroll_x = x + width - scrollbar_width_ - 2;
        int32_t scroll_y = y + 28; // After title
        int32_t scroll_height = height - 32;

        float visible_ratio = static_cast<float>(visible_content_lines_) / static_cast<float>(total_lines);
        int32_t thumb_height = std::max(15, static_cast<int32_t>(scroll_height * visible_ratio));

        int32_t max_scroll = total_lines - visible_content_lines_;
        float scroll_ratio = static_cast<float>(content_scroll_) / static_cast<float>(max_scroll);
        int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_height - thumb_height) * scroll_ratio);

        // Store scrollbar info for hit testing
        content_scrollbar_.track_x = scroll_x;
        content_scrollbar_.track_y = scroll_y;
        content_scrollbar_.track_w = scrollbar_width_;
        content_scrollbar_.track_h = scroll_height;
        content_scrollbar_.thumb_y = thumb_y;
        content_scrollbar_.thumb_h = thumb_height;
        content_scrollbar_.visible = true;

        // Draw track
        rend.draw_rect(scroll_x, scroll_y, scrollbar_width_, scroll_height, sf::Color(20, 22, 28), true);

        // Draw thumb (highlight if dragging)
        sf::Color thumb_color =
            (dragging_ == drag_target::content_scrollbar) ? sf::Color(100, 100, 130) : sf::Color(60, 65, 80);
        rend.draw_rect(scroll_x, thumb_y, scrollbar_width_, thumb_height, thumb_color, true);
    }
    else
    {
        content_scrollbar_.visible = false;
    }
}

std::vector<const yaml_help_topic*> yaml_help_dialog::get_filtered_topics() const
{
    std::vector<const yaml_help_topic*> result;
    for (const auto& topic : topics_)
    {
        if (current_category_ == -1 || topic.category == current_category_)
        {
            result.push_back(&topic);
        }
    }
    return result;
}

std::optional<int32_t> yaml_help_dialog::topic_at(int32_t mx, int32_t my) const
{
    if (mx < topic_area_x_ || mx >= topic_area_x_ + topic_area_w_ || my < topic_area_y_ ||
        my >= topic_area_y_ + topic_area_h_)
    {
        return std::nullopt;
    }

    auto filtered = get_filtered_topics();
    int32_t row = (my - topic_area_y_) / topic_row_height_;
    int32_t idx = topic_scroll_ + row;

    if (idx >= 0 && idx < static_cast<int32_t>(filtered.size()))
    {
        return idx;
    }
    return std::nullopt;
}

std::optional<int32_t> yaml_help_dialog::category_at(int32_t mx, int32_t my) const
{
    if (my < tab_area_y_ || my >= tab_area_y_ + tab_height_)
    {
        return std::nullopt;
    }

    for (size_t i = 0; i < categories_.size(); ++i)
    {
        int32_t tab_x = tab_area_x_ + static_cast<int32_t>(i) * (tab_width_ + 4);
        if (mx >= tab_x && mx < tab_x + tab_width_)
        {
            return static_cast<int32_t>(i);
        }
    }
    return std::nullopt;
}

bool yaml_help_dialog::on_custom_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (btn != sf::Mouse::Button::Left)
        return false;

    // Check close button
    const auto& def = definition();
    auto dlg_bounds = bounds();
    if (def.closeable)
    {
        int32_t close_x = dlg_bounds.x + dlg_bounds.width - 20;
        int32_t close_y = dlg_bounds.y + 4;
        if (x >= close_x && x < close_x + 16 && y >= close_y && y < close_y + 16)
        {
            close();
            return true;
        }
    }

    // Check topic scrollbar
    if (topic_scrollbar_.visible)
    {
        if (x >= topic_scrollbar_.track_x && x < topic_scrollbar_.track_x + topic_scrollbar_.track_w &&
            y >= topic_scrollbar_.track_y && y < topic_scrollbar_.track_y + topic_scrollbar_.track_h)
        {
            auto filtered = get_filtered_topics();
            int32_t max_scroll = std::max(0, static_cast<int32_t>(filtered.size()) - visible_topics_);

            // Check if clicking on thumb (start drag) or track (jump)
            if (y >= topic_scrollbar_.thumb_y && y < topic_scrollbar_.thumb_y + topic_scrollbar_.thumb_h)
            {
                // Start dragging thumb
                dragging_ = drag_target::topic_scrollbar;
                drag_start_y_ = y;
                drag_start_scroll_ = topic_scroll_;
            }
            else
            {
                // Click on track - jump to position
                float click_ratio =
                    static_cast<float>(y - topic_scrollbar_.track_y) / static_cast<float>(topic_scrollbar_.track_h);
                topic_scroll_ = static_cast<int32_t>(click_ratio * (filtered.size() - visible_topics_ + 1));
                topic_scroll_ = std::clamp(topic_scroll_, 0, max_scroll);
            }
            return true;
        }
    }

    // Check content scrollbar
    if (content_scrollbar_.visible)
    {
        if (x >= content_scrollbar_.track_x && x < content_scrollbar_.track_x + content_scrollbar_.track_w &&
            y >= content_scrollbar_.track_y && y < content_scrollbar_.track_y + content_scrollbar_.track_h)
        {
            auto filtered = get_filtered_topics();
            if (selected_topic_ >= 0 && selected_topic_ < static_cast<int32_t>(filtered.size()))
            {
                const auto* topic = filtered[selected_topic_];
                int32_t max_scroll =
                    std::max(0, static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines_);

                // Check if clicking on thumb (start drag) or track (jump)
                if (y >= content_scrollbar_.thumb_y && y < content_scrollbar_.thumb_y + content_scrollbar_.thumb_h)
                {
                    // Start dragging thumb
                    dragging_ = drag_target::content_scrollbar;
                    drag_start_y_ = y;
                    drag_start_scroll_ = content_scroll_;
                }
                else
                {
                    // Click on track - jump to position
                    float click_ratio = static_cast<float>(y - content_scrollbar_.track_y) /
                                        static_cast<float>(content_scrollbar_.track_h);
                    content_scroll_ =
                        static_cast<int32_t>(click_ratio * (topic->content_lines.size() - visible_content_lines_ + 1));
                    content_scroll_ = std::clamp(content_scroll_, 0, max_scroll);
                }
                return true;
            }
        }
    }

    // Category tab click
    auto cat = category_at(x, y);
    if (cat.has_value())
    {
        set_category(cat.value());
        return true;
    }

    // Topic click
    auto topic = topic_at(x, y);
    if (topic.has_value())
    {
        select_topic(topic.value());
        return true;
    }

    return false;
}

bool yaml_help_dialog::on_custom_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    (void)x;
    (void)y;

    if (btn == sf::Mouse::Button::Left && dragging_ != drag_target::none)
    {
        dragging_ = drag_target::none;
        return true;
    }
    return false;
}

bool yaml_help_dialog::on_custom_mouse_move(int32_t x, int32_t y)
{
    // Handle scrollbar dragging
    if (dragging_ == drag_target::topic_scrollbar && topic_scrollbar_.visible)
    {
        auto filtered = get_filtered_topics();
        int32_t max_scroll = std::max(0, static_cast<int32_t>(filtered.size()) - visible_topics_);

        // Calculate scroll based on drag distance
        int32_t drag_range = topic_scrollbar_.track_h - topic_scrollbar_.thumb_h;
        if (drag_range > 0)
        {
            float drag_ratio = static_cast<float>(y - drag_start_y_) / static_cast<float>(drag_range);
            int32_t scroll_delta = static_cast<int32_t>(drag_ratio * max_scroll);
            topic_scroll_ = std::clamp(drag_start_scroll_ + scroll_delta, 0, max_scroll);
        }
        return true;
    }

    if (dragging_ == drag_target::content_scrollbar && content_scrollbar_.visible)
    {
        auto filtered = get_filtered_topics();
        if (selected_topic_ >= 0 && selected_topic_ < static_cast<int32_t>(filtered.size()))
        {
            const auto* topic = filtered[selected_topic_];
            int32_t max_scroll =
                std::max(0, static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines_);

            // Calculate scroll based on drag distance
            int32_t drag_range = content_scrollbar_.track_h - content_scrollbar_.thumb_h;
            if (drag_range > 0)
            {
                float drag_ratio = static_cast<float>(y - drag_start_y_) / static_cast<float>(drag_range);
                int32_t scroll_delta = static_cast<int32_t>(drag_ratio * max_scroll);
                content_scroll_ = std::clamp(drag_start_scroll_ + scroll_delta, 0, max_scroll);
            }
        }
        return true;
    }

    hovered_topic_ = topic_at(x, y);
    hovered_category_ = category_at(x, y);
    return false; // Don't consume, let base class handle drag
}

bool yaml_help_dialog::on_custom_mouse_wheel(int32_t x, int32_t y, int32_t delta)
{
    // Check if mouse is over topic list area
    if (x >= topic_area_x_ && x < topic_area_x_ + topic_area_w_ && y >= topic_area_y_ &&
        y < topic_area_y_ + topic_area_h_)
    {
        // Scroll topic list
        auto filtered = get_filtered_topics();
        int32_t max_scroll = std::max(0, static_cast<int32_t>(filtered.size()) - visible_topics_);

        if (delta > 0)
        {
            // Scroll up
            topic_scroll_ = std::max(0, topic_scroll_ - 1);
        }
        else if (delta < 0)
        {
            // Scroll down
            topic_scroll_ = std::min(topic_scroll_ + 1, max_scroll);
        }
        return true;
    }

    // Check if mouse is over content area
    if (x >= content_area_x_ && x < content_area_x_ + content_area_w_ && y >= content_area_y_ &&
        y < content_area_y_ + content_area_h_)
    {
        // Scroll content
        if (selected_topic_ >= 0)
        {
            auto filtered = get_filtered_topics();
            if (selected_topic_ < static_cast<int32_t>(filtered.size()))
            {
                const auto* topic = filtered[selected_topic_];
                int32_t max_scroll =
                    std::max(0, static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines_);

                if (delta > 0)
                {
                    // Scroll up
                    content_scroll_ = std::max(0, content_scroll_ - 1);
                }
                else if (delta < 0)
                {
                    // Scroll down
                    content_scroll_ = std::min(content_scroll_ + 1, max_scroll);
                }
                return true;
            }
        }
    }

    return false;
}

bool yaml_help_dialog::on_custom_key_press(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Down)
    {
        auto filtered = get_filtered_topics();
        if (selected_topic_ >= 0 && selected_topic_ < static_cast<int32_t>(filtered.size()))
        {
            const auto* topic = filtered[selected_topic_];
            int32_t max_scroll =
                std::max(0, static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines_);
            content_scroll_ = std::min(content_scroll_ + 1, max_scroll);
        }
        return true;
    }

    if (key == sf::Keyboard::Key::Up)
    {
        content_scroll_ = std::max(0, content_scroll_ - 1);
        return true;
    }

    if (key == sf::Keyboard::Key::PageDown)
    {
        auto filtered = get_filtered_topics();
        if (selected_topic_ >= 0 && selected_topic_ < static_cast<int32_t>(filtered.size()))
        {
            const auto* topic = filtered[selected_topic_];
            int32_t max_scroll =
                std::max(0, static_cast<int32_t>(topic->content_lines.size()) - visible_content_lines_);
            content_scroll_ = std::min(content_scroll_ + visible_content_lines_, max_scroll);
        }
        return true;
    }

    if (key == sf::Keyboard::Key::PageUp)
    {
        content_scroll_ = std::max(0, content_scroll_ - visible_content_lines_);
        return true;
    }

    return false;
}

} // namespace hb
