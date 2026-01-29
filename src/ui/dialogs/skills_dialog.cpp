#include "ui/dialogs/skills_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <format>

namespace hb {

skills_dialog::skills_dialog()
    : dialog(dialog_type::skills) {
    set_title("Skills");
    set_bounds({100, 80, 300, 380});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void skills_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
}

void skills_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Category tabs
    const char* categories[] = {"All", "Combat", "Magic", "Craft", "Misc"};
    for (int i = 0; i < 5; ++i) {
        int32_t tab_x = x + i * 56;
        bool is_current = (static_cast<int>(current_category_) == i);

        sf::Color bg = is_current ? sf::Color(80, 80, 120) : sf::Color(50, 50, 70);
        rend.draw_rect(tab_x, y, 52, 18, bg, true);
        rend.draw_rect(tab_x, y, 52, 18, sf::Color(80, 80, 100), false);

        sf::Color text_color = is_current ? sf::Color::Yellow : sf::Color::White;
        rend.draw_text(categories[i], tab_x + 4, y + 2, text_color, 11);
    }
    y += 26;

    // Separator
    rend.draw_line(x, y, x + 280, y, sf::Color(80, 80, 100));
    y += 8;

    content_start_y_ = y;

    // Header row
    rend.draw_text("Skill", x, y, sf::Color(180, 180, 200), 12);
    rend.draw_text("Level", x + 140, y, sf::Color(180, 180, 200), 12);
    rend.draw_text("Mastery", x + 190, y, sf::Color(180, 180, 200), 12);
    y += 18;

    rend.draw_line(x, y, x + 280, y, sf::Color(60, 60, 80));
    y += 4;

    // Filter skills by category
    filtered_skills_.clear();
    for (const auto& sk : skills_) {
        bool include = false;
        switch (current_category_) {
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
        if (include) {
            filtered_skills_.push_back(sk);
        }
    }

    if (filtered_skills_.empty()) {
        rend.draw_text("No skills in this category", x, y, sf::Color(150, 150, 150));
        return;
    }

    // Draw skill rows
    int32_t max_visible = std::min(static_cast<int32_t>(filtered_skills_.size()) - scroll_offset_,
                                   visible_rows);

    for (int32_t i = 0; i < max_visible; ++i) {
        size_t idx = static_cast<size_t>(scroll_offset_ + i);
        bool hovered = hovered_index_.has_value() && hovered_index_.value() == idx;
        render_skill_row(rend, filtered_skills_[idx], y, hovered);
        y += row_height;
    }

    // Scroll indicator
    if (filtered_skills_.size() > static_cast<size_t>(visible_rows)) {
        int32_t scroll_y = content_start_y_ + 22;
        int32_t scroll_height = visible_rows * row_height;
        int32_t thumb_height = static_cast<int32_t>(scroll_height *
            static_cast<float>(visible_rows) / filtered_skills_.size());
        int32_t thumb_y = scroll_y + static_cast<int32_t>((scroll_height - thumb_height) *
            static_cast<float>(scroll_offset_) / (filtered_skills_.size() - visible_rows));

        rend.draw_rect(bounds_.x + bounds_.width - 18, scroll_y, 8, scroll_height,
                      sf::Color(40, 40, 50), true);
        rend.draw_rect(bounds_.x + bounds_.width - 18, thumb_y, 8, thumb_height,
                      sf::Color(80, 80, 100), true);
    }
}

void skills_dialog::render_skill_row(renderer& rend, const skill& sk,
                                     int32_t y, bool hovered) {
    int32_t x = bounds_.x + 10;

    // Row highlight
    if (hovered) {
        rend.draw_rect(x - 2, y - 2, 276, row_height - 2, sf::Color(50, 50, 70), true);
    }

    // Skill name
    rend.draw_text(sk.name, x, y, sf::Color::White, 12);

    // Skill level
    rend.draw_text(std::format("Lv {}", sk.level()), x + 140, y, sf::Color(200, 200, 255), 12);

    // Mastery bar
    int32_t bar_x = x + 190;
    int32_t bar_width = 70;
    int32_t bar_height = 12;

    rend.draw_rect(bar_x, y + 2, bar_width, bar_height, sf::Color(30, 30, 40), true);

    float mastery_pct = sk.progress() / 100.0f;
    int32_t fill_width = static_cast<int32_t>(bar_width * mastery_pct);

    sf::Color mastery_color = mastery_pct >= 1.0f ? sf::Color(255, 200, 100)  // Gold for 100%
                            : mastery_pct >= 0.8f ? sf::Color(100, 200, 100)
                            : sf::Color(100, 150, 200);

    if (fill_width > 0) {
        rend.draw_rect(bar_x, y + 2, fill_width, bar_height, mastery_color, true);
    }

    rend.draw_rect(bar_x, y + 2, bar_width, bar_height, sf::Color(60, 60, 80), false);

    // Mastery percentage
    std::string pct_text = std::format("{:.0f}%", sk.progress());
    int32_t text_x = bar_x + bar_width / 2 - static_cast<int32_t>(pct_text.length() * 2);
    rend.draw_text(pct_text, text_x, y + 2, sf::Color::White, 10);
}

std::optional<size_t> skills_dialog::skill_index_at(int32_t x, int32_t y) const {
    if (x < bounds_.x + 8 || x > bounds_.x + bounds_.width - 20) {
        return std::nullopt;
    }

    int32_t row_start_y = content_start_y_ + 22;
    if (y < row_start_y) {
        return std::nullopt;
    }

    int32_t row = (y - row_start_y) / row_height;
    if (row < 0 || row >= visible_rows) {
        return std::nullopt;
    }

    size_t idx = static_cast<size_t>(scroll_offset_ + row);
    if (idx >= filtered_skills_.size()) {
        return std::nullopt;
    }

    return idx;
}

bool skills_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check category tabs
    int32_t tab_y = bounds_.y + 32;
    for (int i = 0; i < 5; ++i) {
        int32_t tab_x = bounds_.x + 10 + i * 56;
        ui_rect tab_rect{tab_x, tab_y, 52, 18};
        if (tab_rect.contains(x, y)) {
            set_category(static_cast<filter_category>(i));
            return true;
        }
    }

    // Check skill rows
    if (btn == sf::Mouse::Button::Left) {
        auto idx = skill_index_at(x, y);
        if (idx.has_value()) {
            if (on_skill_click_) {
                on_skill_click_(filtered_skills_[idx.value()].id);
            }
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool skills_dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (!visible_) return false;

    hovered_index_ = skill_index_at(x, y);
    return dialog::handle_mouse_move(x, y);
}

void skills_dialog::set_skills(const std::vector<skill>& skills) {
    skills_ = skills;
    scroll_offset_ = 0;
}

void skills_dialog::update_skill(uint16_t skill_id, uint32_t experience) {
    for (auto& sk : skills_) {
        if (sk.id == skill_id) {
            sk.experience = experience;
            break;
        }
    }
}

void skills_dialog::clear_skills() {
    skills_.clear();
    filtered_skills_.clear();
    scroll_offset_ = 0;
}

void skills_dialog::set_category(filter_category cat) {
    current_category_ = cat;
    scroll_offset_ = 0;
    hovered_index_.reset();
}

} // namespace hb
