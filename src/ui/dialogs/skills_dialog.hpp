#pragma once

#include "ui/ui_system.hpp"
#include "gameplay/skills.hpp"
#include <functional>
#include <vector>
#include <optional>

namespace hb {

// Skills dialog - displays skill levels and progress
class skills_dialog : public dialog {
public:
    skills_dialog();
    ~skills_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set skills data
    void set_skills(const std::vector<skill>& skills);
    void update_skill(uint16_t skill_id, uint32_t experience);
    void clear_skills();

    // Category filter
    enum class filter_category {
        all,
        combat,
        magic,
        crafting,
        misc
    };

    void set_category(filter_category cat);
    filter_category current_category() const { return current_category_; }

    // Callbacks
    using skill_callback = std::function<void(uint16_t skill_id)>;
    void set_on_skill_click(skill_callback callback) { on_skill_click_ = std::move(callback); }

private:
    void rebuild_filtered();
    void render_skill_row(renderer& rend, const skill& sk, int32_t y, bool hovered);
    std::optional<size_t> skill_index_at(int32_t x, int32_t y) const;
    void clamp_scroll();

    std::vector<skill> skills_;
    std::vector<skill> filtered_skills_;

    filter_category current_category_ = filter_category::all;
    std::optional<size_t> hovered_index_;
    int32_t scroll_offset_ = 0;

    skill_callback on_skill_click_;

    // Each row: skill name + level on first line, progress bar on second line
    static constexpr int32_t row_height = 36;
    static constexpr int32_t visible_rows = 8;
    int32_t content_start_y_ = 0;
};

} // namespace hb
