#include "ui/dialogs/skills_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <format>

namespace hb {

skills_dialog::skills_dialog()
    : dialog(dialog_type::skills)
{
    set_title("Skills");
    set_bounds({100, 80, 280, 370});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
    set_drag_clamp(drag_clamp::partial);
}

void skills_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);

    if (!visible_) return;

    // Handle scroll wheel when mouse is over dialog
    int32_t wheel = inp.wheel_delta();
    if (wheel != 0)
    {
        int32_t mx = inp.mouse_x();
        int32_t my = inp.mouse_y();
        if (bounds_.contains(mx, my))
        {
            scroll_offset_ -= wheel;
            clamp_scroll();
        }
    }
}

void skills_dialog::render(renderer& rend)
{
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Category tabs
    const char* categories[] = {"All", "Combat", "Magic", "Craft", "Misc"};
    int32_t tab_width = 50;
    for (int i = 0; i < 5; ++i)
    {
        int32_t tab_x = x + i * (tab_width + 2);
        bool is_current = (static_cast<int>(current_category_) == i);

        sf::Color bg = is_current ? sf::Color(80, 80, 120) : sf::Color(50, 50, 70);
        rend.draw_rect(tab_x, y, tab_width, 18, bg, true);
        rend.draw_rect(tab_x, y, tab_width, 18, sf::Color(80, 80, 100), false);

        sf::Color text_color = is_current ? sf::Color::Yellow : sf::Color::White;
        rend.draw_text(categories[i], tab_x + 4, y + 2, text_color, 11);
    }
    y += 24;

    // Separator
    rend.draw_line(x, y, x + 260, y, sf::Color(80, 80, 100));
    y += 6;

    content_start_y_ = y;

    // Rebuild filtered list
    rebuild_filtered();

    if (filtered_skills_.empty())
    {
        rend.draw_text("No skills", x, y, sf::Color(150, 150, 150));
        return;
    }

    // Draw skill rows
    int32_t max_visible = std::min(static_cast<int32_t>(filtered_skills_.size()) - scroll_offset_,
                                   visible_rows);

    for (int32_t i = 0; i < max_visible; ++i)
    {
        size_t idx = static_cast<size_t>(scroll_offset_ + i);
        bool hovered = hovered_index_.has_value() && hovered_index_.value() == idx;
        render_skill_row(rend, filtered_skills_[idx], y, hovered);
        y += row_height;
    }

    // Scroll indicator
    if (static_cast<int32_t>(filtered_skills_.size()) > visible_rows)
    {
        int32_t scroll_x = bounds_.x + bounds_.width - 14;
        int32_t scroll_y = content_start_y_;
        int32_t scroll_height = visible_rows * row_height;
        float ratio = static_cast<float>(visible_rows) / static_cast<float>(filtered_skills_.size());
        int32_t thumb_height = std::max(16, static_cast<int32_t>(scroll_height * ratio));
        int32_t max_scroll = static_cast<int32_t>(filtered_skills_.size()) - visible_rows;
        float scroll_pct = max_scroll > 0 ? static_cast<float>(scroll_offset_) / max_scroll : 0.0f;
        int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_height - thumb_height) * scroll_pct);

        rend.draw_rect(scroll_x, scroll_y, 6, scroll_height, sf::Color(40, 40, 50), true);
        rend.draw_rect(scroll_x, thumb_y, 6, thumb_height, sf::Color(100, 100, 120), true);
    }
}

void skills_dialog::render_skill_row(renderer& rend, const skill& sk,
                                     int32_t y, bool hovered)
{
    int32_t x = bounds_.x + 10;
    int32_t row_width = bounds_.width - 28;

    // Row background highlight
    if (hovered)
    {
        rend.draw_rect(x - 2, y - 1, row_width + 4, row_height - 2, sf::Color(50, 50, 70), true);
    }

    // First line: skill name (left) and level (right)
    uint8_t lvl = sk.level();

    // Skill name color based on level
    sf::Color name_color = lvl == 0 ? sf::Color(120, 120, 120) : sf::Color::White;
    rend.draw_text(sk.name, x, y, name_color, 12);

    // Level text on the right
    sf::Color level_color;
    if (lvl >= 100)
        level_color = sf::Color(255, 200, 80);   // Gold for mastered
    else if (lvl >= 80)
        level_color = sf::Color(100, 200, 100);   // Green for high
    else
        level_color = sf::Color(180, 180, 220);   // Light blue-gray

    std::string level_text = std::format("{}", lvl);
    rend.draw_text(level_text, x + row_width - 24, y, level_color, 12);

    // Second line: progress bar
    int32_t bar_x = x;
    int32_t bar_y = y + 16;
    int32_t bar_width = row_width;
    int32_t bar_height = 10;

    // Background
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(25, 25, 35), true);

    // Fill based on sub-level progress
    float fill_pct = sk.sub_progress;
    if (lvl >= 100)
        fill_pct = 1.0f;  // Mastered = full bar

    int32_t fill_width = static_cast<int32_t>(bar_width * fill_pct);

    if (fill_width > 0)
    {
        sf::Color bar_color;
        if (lvl >= 100)
            bar_color = sf::Color(200, 160, 60);   // Gold
        else if (lvl >= 80)
            bar_color = sf::Color(60, 160, 60);     // Green
        else
            bar_color = sf::Color(60, 100, 180);    // Blue

        rend.draw_rect(bar_x, bar_y, fill_width, bar_height, bar_color, true);
    }

    // Bar border
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(60, 60, 80), false);

    // Progress text centered on bar
    std::string pct_text;
    if (lvl >= 100)
        pct_text = "MAX";
    else if (fill_pct > 0.0f)
        pct_text = std::format("{:.0f}%", fill_pct * 100.0f);

    if (!pct_text.empty())
    {
        int32_t text_w = static_cast<int32_t>(pct_text.length()) * 5;
        rend.draw_text(pct_text, bar_x + (bar_width - text_w) / 2, bar_y, sf::Color(220, 220, 220), 9);
    }

    // Separator line
    rend.draw_line(x, y + row_height - 2, x + row_width, y + row_height - 2, sf::Color(40, 40, 55));
}

void skills_dialog::rebuild_filtered()
{
    filtered_skills_.clear();
    for (const auto& sk : skills_)
    {
        bool include = false;
        switch (current_category_)
        {
            case filter_category::all:
                include = true;
                break;
            case filter_category::combat:
                include = (sk.category == skill_category::combat);
                break;
            case filter_category::magic:
                include = (sk.category == skill_category::magic);
                break;
            case filter_category::crafting:
                include = (sk.category == skill_category::crafting);
                break;
            case filter_category::misc:
                include = (sk.category == skill_category::misc ||
                          sk.category == skill_category::gathering);
                break;
        }
        if (include)
            filtered_skills_.push_back(sk);
    }
}

std::optional<size_t> skills_dialog::skill_index_at(int32_t x, int32_t y) const
{
    if (x < bounds_.x + 8 || x > bounds_.x + bounds_.width - 20)
        return std::nullopt;

    if (y < content_start_y_)
        return std::nullopt;

    int32_t row = (y - content_start_y_) / row_height;
    if (row < 0 || row >= visible_rows)
        return std::nullopt;

    size_t idx = static_cast<size_t>(scroll_offset_ + row);
    if (idx >= filtered_skills_.size())
        return std::nullopt;

    return idx;
}

bool skills_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_) return false;

    // Check category tabs
    int32_t tab_y = bounds_.y + 32;
    int32_t tab_width = 50;
    for (int i = 0; i < 5; ++i)
    {
        int32_t tab_x = bounds_.x + 10 + i * (tab_width + 2);
        ui_rect tab_rect{tab_x, tab_y, tab_width, 18};
        if (tab_rect.contains(x, y))
        {
            set_category(static_cast<filter_category>(i));
            return true;
        }
    }

    // Check skill rows
    if (btn == sf::Mouse::Button::Left)
    {
        auto idx = skill_index_at(x, y);
        if (idx.has_value())
        {
            if (on_skill_click_)
                on_skill_click_(filtered_skills_[idx.value()].id);
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool skills_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_) return false;

    hovered_index_ = skill_index_at(x, y);
    return dialog::handle_mouse_move(x, y);
}

void skills_dialog::set_skills(const std::vector<skill>& skills)
{
    skills_ = skills;

    // Sort by category then by id
    std::sort(skills_.begin(), skills_.end(), [](const skill& a, const skill& b) {
        if (a.category != b.category)
            return static_cast<uint8_t>(a.category) < static_cast<uint8_t>(b.category);
        return a.id < b.id;
    });

    clamp_scroll();
}

void skills_dialog::update_skill(uint16_t skill_id, uint32_t experience)
{
    for (auto& sk : skills_)
    {
        if (sk.id == skill_id)
        {
            sk.experience = experience;
            break;
        }
    }
}

void skills_dialog::clear_skills()
{
    skills_.clear();
    filtered_skills_.clear();
    scroll_offset_ = 0;
}

void skills_dialog::set_category(filter_category cat)
{
    current_category_ = cat;
    scroll_offset_ = 0;
    hovered_index_.reset();
}

void skills_dialog::clamp_scroll()
{
    rebuild_filtered();
    int32_t max_scroll = std::max(0, static_cast<int32_t>(filtered_skills_.size()) - visible_rows);
    scroll_offset_ = std::clamp(scroll_offset_, 0, max_scroll);
}

} // namespace hb
