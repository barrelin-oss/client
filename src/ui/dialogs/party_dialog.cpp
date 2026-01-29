#include "ui/dialogs/party_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>
#include <algorithm>

namespace hb {

// ============ party_dialog ============

party_dialog::party_dialog()
    : dialog(dialog_type::party) {
    set_title("Party");
    set_bounds({20, 120, 200, 280});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void party_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
}

void party_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    if (members_.empty()) {
        rend.draw_text("Not in a party", x, y, sf::Color(150, 150, 150));
        return;
    }

    // Party size
    rend.draw_text(std::format("Members: {}/{}", members_.size(), max_party_size),
                  x, y, sf::Color(180, 180, 200), 12);
    y += 20;

    content_start_y_ = y;

    // Draw member rows
    for (size_t i = 0; i < members_.size(); ++i) {
        bool hovered = hovered_index_.has_value() && hovered_index_.value() == i;
        render_member_row(rend, members_[i], y, hovered);
        y += member_row_height;
    }

    // Leave button
    y = bounds_.y + bounds_.height - 35;
    rend.draw_rect(x + 55, y, 80, 24, sf::Color(100, 60, 60), true);
    rend.draw_text("Leave", x + 75, y + 4, sf::Color::White);
}

void party_dialog::render_member_row(renderer& rend, const party_member_info& member,
                                     int32_t y, bool hovered) {
    int32_t x = bounds_.x + 10;

    // Row background
    if (hovered) {
        rend.draw_rect(x - 2, y - 2, 184, member_row_height - 4, sf::Color(50, 50, 70), true);
    }

    // Leader indicator
    if (member.is_leader) {
        rend.draw_text("*", x, y, sf::Color::Yellow);
    }

    // Name and level
    sf::Color name_color = member.is_online ? sf::Color::White : sf::Color(100, 100, 100);
    rend.draw_text(member.name, x + 12, y, name_color, 12);
    rend.draw_text(std::format("Lv{}", member.level), x + 130, y, sf::Color(180, 180, 200), 10);

    // HP bar
    int32_t bar_y = y + 16;
    int32_t bar_width = 120;
    int32_t bar_height = 8;

    rend.draw_rect(x + 12, bar_y, bar_width, bar_height, sf::Color(30, 30, 40), true);

    float hp_pct = member.max_hp > 0 ? static_cast<float>(member.hp) / member.max_hp : 0.0f;
    int32_t hp_width = static_cast<int32_t>(bar_width * hp_pct);
    sf::Color hp_color = hp_pct > 0.5f ? sf::Color(100, 180, 100)
                       : hp_pct > 0.2f ? sf::Color(200, 200, 100)
                       : sf::Color(200, 100, 100);

    if (hp_width > 0) {
        rend.draw_rect(x + 12, bar_y, hp_width, bar_height, hp_color, true);
    }

    // MP bar (smaller)
    bar_y += 10;
    bar_height = 6;
    rend.draw_rect(x + 12, bar_y, bar_width, bar_height, sf::Color(30, 30, 40), true);

    float mp_pct = member.max_mp > 0 ? static_cast<float>(member.mp) / member.max_mp : 0.0f;
    int32_t mp_width = static_cast<int32_t>(bar_width * mp_pct);

    if (mp_width > 0) {
        rend.draw_rect(x + 12, bar_y, mp_width, bar_height, sf::Color(80, 80, 180), true);
    }
}

std::optional<size_t> party_dialog::member_index_at(int32_t x, int32_t y) const {
    if (x < bounds_.x + 8 || x > bounds_.x + bounds_.width - 8) {
        return std::nullopt;
    }

    if (y < content_start_y_) {
        return std::nullopt;
    }

    int32_t idx = (y - content_start_y_) / member_row_height;
    if (idx < 0 || static_cast<size_t>(idx) >= members_.size()) {
        return std::nullopt;
    }

    return static_cast<size_t>(idx);
}

bool party_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check leave button
    int32_t leave_y = bounds_.y + bounds_.height - 35;
    ui_rect leave_btn{bounds_.x + 65, leave_y, 80, 24};
    if (leave_btn.contains(x, y)) {
        if (on_leave_) {
            on_leave_();
        }
        return true;
    }

    // Right-click for context menu on member
    if (btn == sf::Mouse::Button::Right) {
        auto idx = member_index_at(x, y);
        if (idx.has_value()) {
            selected_index_ = idx;
            // Would show context menu here
            return true;
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool party_dialog::handle_mouse_move(int32_t x, int32_t y) {
    if (!visible_) return false;

    hovered_index_ = member_index_at(x, y);
    return dialog::handle_mouse_move(x, y);
}

void party_dialog::set_members(const std::vector<party_member_info>& members) {
    members_ = members;
}

void party_dialog::update_member(const party_member_info& member) {
    for (auto& m : members_) {
        if (m.entity_id == member.entity_id) {
            m = member;
            return;
        }
    }
    // Not found, add
    members_.push_back(member);
}

void party_dialog::remove_member(uint32_t entity_id) {
    members_.erase(
        std::remove_if(members_.begin(), members_.end(),
            [entity_id](const party_member_info& m) { return m.entity_id == entity_id; }),
        members_.end()
    );
}

void party_dialog::clear_members() {
    members_.clear();
}

// ============ guild_dialog ============

guild_dialog::guild_dialog()
    : dialog(dialog_type::guild) {
    set_title("Guild");
    set_bounds({
        static_cast<int32_t>(screen_width) / 2 - 175,
        static_cast<int32_t>(screen_height) / 2 - 150,
        350,
        300
    });
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void guild_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);
}

void guild_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    if (guild_name_.empty()) {
        rend.draw_text("Not in a guild", x, y, sf::Color(150, 150, 150));
        return;
    }

    // Guild name
    rend.draw_text(guild_name_, x, y, sf::Color::Yellow);
    y += 22;

    // Guild master
    rend.draw_text(std::format("Master: {}", guild_master_), x, y, sf::Color::White, 12);
    y += 18;

    // Members count
    rend.draw_text(std::format("Members: {}/{}", member_count_, max_members_), x, y, sf::Color(180, 180, 200), 12);

    // Guild gold
    rend.draw_text(std::format("Treasury: {}", guild_gold_), x + 150, y, sf::Color(255, 220, 100), 12);
    y += 22;

    // Separator
    rend.draw_line(x, y, x + 330, y, sf::Color(60, 60, 80));
    y += 8;

    // Column headers
    rend.draw_text("Name", x, y, sf::Color(150, 150, 180), 11);
    rend.draw_text("Rank", x + 140, y, sf::Color(150, 150, 180), 11);
    rend.draw_text("Lvl", x + 220, y, sf::Color(150, 150, 180), 11);
    rend.draw_text("Status", x + 270, y, sf::Color(150, 150, 180), 11);
    y += 16;

    // Member list
    int32_t end_idx = std::min(scroll_offset_ + visible_members, static_cast<int32_t>(members_.size()));
    for (int32_t i = scroll_offset_; i < end_idx; ++i) {
        const auto& member = members_[i];

        sf::Color name_color = member.is_online ? sf::Color::White : sf::Color(100, 100, 100);

        rend.draw_text(member.name, x, y, name_color, 11);
        rend.draw_text(member.rank, x + 140, y, sf::Color(180, 180, 200), 11);
        rend.draw_text(std::to_string(member.level), x + 220, y, sf::Color(200, 200, 255), 11);

        sf::Color status_color = member.is_online ? sf::Color(100, 255, 100) : sf::Color(150, 150, 150);
        rend.draw_text(member.is_online ? "Online" : "Offline", x + 270, y, status_color, 11);

        y += member_row_height;
    }

    // Buttons
    int32_t btn_y = bounds_.y + bounds_.height - 40;

    rend.draw_rect(x + 60, btn_y, 100, 26, sf::Color(60, 80, 100), true);
    rend.draw_text("Guild Bank", x + 73, btn_y + 5, sf::Color::White, 11);

    rend.draw_rect(x + 180, btn_y, 100, 26, sf::Color(100, 60, 60), true);
    rend.draw_text("Leave Guild", x + 190, btn_y + 5, sf::Color::White, 11);
}

bool guild_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    int32_t base_x = bounds_.x + 10;
    int32_t btn_y = bounds_.y + bounds_.height - 40;

    ui_rect bank_btn{base_x + 60, btn_y, 100, 26};
    ui_rect leave_btn{base_x + 180, btn_y, 100, 26};

    if (bank_btn.contains(x, y)) {
        if (on_open_bank_) on_open_bank_();
        return true;
    }

    if (leave_btn.contains(x, y)) {
        if (on_leave_guild_) on_leave_guild_();
        return true;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

void guild_dialog::set_members(const std::vector<guild_member_info>& members) {
    members_ = members;
    scroll_offset_ = 0;
}

void guild_dialog::clear_members() {
    members_.clear();
    scroll_offset_ = 0;
}

} // namespace hb
