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
        static_cast<int32_t>(screen_height) / 2 - 175,
        350,
        350
    });
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void guild_dialog::update(float delta_time, const input& inp) {
    dialog::update(delta_time, inp);

    if (has_pending_invite_)
    {
        invite_time_remaining_ -= delta_time;
        if (invite_time_remaining_ <= 0.0f)
        {
            has_pending_invite_ = false;
        }
    }
}

void guild_dialog::render(renderer& rend) {
    if (!visible_) return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Pending invite banner always shown at top if present
    if (has_pending_invite_)
    {
        render_invite_banner(rend, x, y);
        y += 52;
    }

    switch (mode_)
    {
        case guild_dialog_mode::no_guild:
        case guild_dialog_mode::creating:
            render_no_guild(rend, x, y);
            break;
        case guild_dialog_mode::guild_view:
            render_guild_view(rend, x, y);
            break;
    }
}

void guild_dialog::render_invite_banner(renderer& rend, int32_t x, int32_t y)
{
    // Background
    rend.draw_rect(x - 2, y - 2, 334, 48, sf::Color(40, 60, 40), true);
    rend.draw_rect(x - 2, y - 2, 334, 48, sf::Color(80, 140, 80), false);

    std::string text = invite_from_ + " invites you to [" + invite_guild_tag_ + "] " + invite_guild_name_;
    rend.draw_text(text, x + 2, y, sf::Color(180, 255, 180), 11);

    int32_t btn_y = y + 22;

    // Accept button
    rend.draw_rect(x + 60, btn_y, 80, 20, sf::Color(60, 120, 60), true);
    rend.draw_text("Accept", x + 76, btn_y + 2, sf::Color::White, 11);

    // Decline button
    rend.draw_rect(x + 160, btn_y, 80, 20, sf::Color(120, 60, 60), true);
    rend.draw_text("Decline", x + 174, btn_y + 2, sf::Color::White, 11);

    // Timer
    rend.draw_text(std::format("{:.0f}s", invite_time_remaining_),
                  x + 260, btn_y + 2, sf::Color(180, 180, 180), 10);
}

void guild_dialog::render_no_guild(renderer& rend, int32_t x, int32_t y)
{
    rend.draw_text("Not in a guild", x, y, sf::Color(150, 150, 150));
    y += 30;

    rend.draw_text("Create a Guild", x, y, sf::Color(220, 200, 120), 13);
    y += 24;

    // Guild name input
    bool name_active = (active_field_ == text_field::create_name);
    rend.draw_text("Name:", x, y + 2, sf::Color(180, 180, 200), 11);
    rend.draw_rect(x + 50, y, 220, 20,
                  name_active ? sf::Color(60, 60, 90) : sf::Color(40, 40, 55), true);
    rend.draw_rect(x + 50, y, 220, 20,
                  name_active ? sf::Color(120, 120, 180) : sf::Color(70, 70, 90), false);
    std::string name_display = create_name_input_;
    if (name_active) name_display += "_";
    rend.draw_text(name_display, x + 54, y + 2, sf::Color::White, 11);
    y += 28;

    // Guild tag input
    bool tag_active = (active_field_ == text_field::create_tag);
    rend.draw_text("Tag:", x, y + 2, sf::Color(180, 180, 200), 11);
    rend.draw_rect(x + 50, y, 80, 20,
                  tag_active ? sf::Color(60, 60, 90) : sf::Color(40, 40, 55), true);
    rend.draw_rect(x + 50, y, 80, 20,
                  tag_active ? sf::Color(120, 120, 180) : sf::Color(70, 70, 90), false);
    std::string tag_display = create_tag_input_;
    if (tag_active) tag_display += "_";
    rend.draw_text(tag_display, x + 54, y + 2, sf::Color::White, 11);
    y += 32;

    // Create button
    rend.draw_rect(x + 100, y, 120, 26, sf::Color(60, 100, 60), true);
    rend.draw_text("Create Guild", x + 115, y + 5, sf::Color::White, 11);
}

void guild_dialog::render_guild_view(renderer& rend, int32_t x, int32_t y)
{
    // Guild header: name [tag]
    std::string header = guild_name_;
    if (!guild_tag_.empty()) header += " [" + guild_tag_ + "]";
    rend.draw_text(header, x, y, sf::Color::Yellow);
    y += 20;

    // Master
    rend.draw_text(std::format("Master: {}", guild_master_), x, y, sf::Color::White, 11);
    y += 16;

    // MOTD
    if (!motd_.empty())
    {
        rend.draw_text("MOTD: " + motd_, x, y, sf::Color(180, 200, 180), 10);
        y += 14;
    }

    y += 4;

    // Separator
    rend.draw_line(x, y, x + 330, y, sf::Color(60, 60, 80));
    y += 6;

    // Column headers
    rend.draw_text("Name", x, y, sf::Color(150, 150, 180), 10);
    rend.draw_text("Rank", x + 160, y, sf::Color(150, 150, 180), 10);
    rend.draw_text("Status", x + 260, y, sf::Color(150, 150, 180), 10);
    y += 16;

    // Member list
    int32_t end_idx = std::min(scroll_offset_ + visible_members, static_cast<int32_t>(members_.size()));
    for (int32_t i = scroll_offset_; i < end_idx; ++i)
    {
        const auto& member = members_[i];

        sf::Color name_color = member.is_online ? sf::Color::White : sf::Color(100, 100, 100);
        rend.draw_text(member.name, x, y, name_color, 11);
        rend.draw_text(member.rank_name, x + 160, y, sf::Color(180, 180, 200), 11);

        sf::Color status_color = member.is_online ? sf::Color(100, 255, 100) : sf::Color(150, 150, 150);
        rend.draw_text(member.is_online ? "Online" : "Offline", x + 260, y, status_color, 11);

        y += member_row_height;
    }

    // Bottom section: invite input + buttons
    int32_t bottom_y = bounds_.y + bounds_.height - 70;

    // Invite input
    bool invite_active = (active_field_ == text_field::invite_target);
    rend.draw_text("Invite:", x, bottom_y + 2, sf::Color(180, 180, 200), 11);
    rend.draw_rect(x + 50, bottom_y, 150, 20,
                  invite_active ? sf::Color(60, 60, 90) : sf::Color(40, 40, 55), true);
    rend.draw_rect(x + 50, bottom_y, 150, 20,
                  invite_active ? sf::Color(120, 120, 180) : sf::Color(70, 70, 90), false);
    std::string inv_display = invite_input_;
    if (invite_active) inv_display += "_";
    rend.draw_text(inv_display, x + 54, bottom_y + 2, sf::Color::White, 11);

    // Invite button
    rend.draw_rect(x + 210, bottom_y, 60, 20, sf::Color(60, 100, 60), true);
    rend.draw_text("Send", x + 224, bottom_y + 2, sf::Color::White, 11);

    bottom_y += 30;

    // Action buttons based on rank
    if (local_rank_ == 0) // Master
    {
        rend.draw_rect(x, bottom_y, 90, 24, sf::Color(60, 80, 100), true);
        rend.draw_text("Set MOTD", x + 10, bottom_y + 5, sf::Color::White, 11);

        rend.draw_rect(x + 120, bottom_y, 90, 24, sf::Color(100, 60, 60), true);
        rend.draw_text("Disband", x + 138, bottom_y + 5, sf::Color::White, 11);
    }
    else
    {
        rend.draw_rect(x + 120, bottom_y, 90, 24, sf::Color(100, 60, 60), true);
        rend.draw_text("Leave", x + 142, bottom_y + 5, sf::Color::White, 11);
    }
}

bool guild_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) {
    if (!visible_) return false;

    // Check invite banner clicks first
    if (has_pending_invite_)
    {
        if (handle_click_invite_banner(x, y))
            return true;
    }

    switch (mode_)
    {
        case guild_dialog_mode::no_guild:
        case guild_dialog_mode::creating:
            if (handle_click_no_guild(x, y))
                return true;
            break;
        case guild_dialog_mode::guild_view:
            if (handle_click_guild_view(x, y))
                return true;
            break;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool guild_dialog::handle_click_invite_banner(int32_t x, int32_t y)
{
    int32_t bx = bounds_.x + 10;
    int32_t by = bounds_.y + 32 + 22;  // banner button row

    ui_rect accept_btn{bx + 60, by, 80, 20};
    ui_rect decline_btn{bx + 160, by, 80, 20};

    if (accept_btn.contains(x, y))
    {
        if (on_accept_invite_) on_accept_invite_();
        return true;
    }

    if (decline_btn.contains(x, y))
    {
        if (on_decline_invite_) on_decline_invite_();
        return true;
    }

    return false;
}

bool guild_dialog::handle_click_no_guild(int32_t x, int32_t y)
{
    int32_t bx = bounds_.x + 10;
    int32_t by = bounds_.y + 32;
    if (has_pending_invite_) by += 52;

    // Name input field
    int32_t name_y = by + 54;
    ui_rect name_field{bx + 50, name_y, 220, 20};
    if (name_field.contains(x, y))
    {
        active_field_ = text_field::create_name;
        return true;
    }

    // Tag input field
    int32_t tag_y = name_y + 28;
    ui_rect tag_field{bx + 50, tag_y, 80, 20};
    if (tag_field.contains(x, y))
    {
        active_field_ = text_field::create_tag;
        return true;
    }

    // Create button
    int32_t create_y = tag_y + 32;
    ui_rect create_btn{bx + 100, create_y, 120, 26};
    if (create_btn.contains(x, y))
    {
        if (on_create_ && !create_name_input_.empty() && !create_tag_input_.empty())
        {
            on_create_(create_name_input_, create_tag_input_);
            create_name_input_.clear();
            create_tag_input_.clear();
        }
        return true;
    }

    active_field_ = text_field::none;
    return false;
}

bool guild_dialog::handle_click_guild_view(int32_t x, int32_t y)
{
    int32_t bx = bounds_.x + 10;
    int32_t bottom_y = bounds_.y + bounds_.height - 70;

    // Invite input field
    ui_rect invite_field{bx + 50, bottom_y, 150, 20};
    if (invite_field.contains(x, y))
    {
        active_field_ = text_field::invite_target;
        return true;
    }

    // Send invite button
    ui_rect send_btn{bx + 210, bottom_y, 60, 20};
    if (send_btn.contains(x, y))
    {
        if (on_invite_ && !invite_input_.empty())
        {
            on_invite_(invite_input_);
            invite_input_.clear();
        }
        return true;
    }

    bottom_y += 30;

    if (local_rank_ == 0) // Master buttons
    {
        ui_rect motd_btn{bx, bottom_y, 90, 24};
        ui_rect disband_btn{bx + 120, bottom_y, 90, 24};

        if (motd_btn.contains(x, y))
        {
            // Toggle MOTD input mode
            if (active_field_ == text_field::motd)
            {
                if (on_set_motd_ && !motd_input_.empty())
                {
                    on_set_motd_(motd_input_);
                    motd_input_.clear();
                }
                active_field_ = text_field::none;
            }
            else
            {
                active_field_ = text_field::motd;
                motd_input_ = motd_;
            }
            return true;
        }

        if (disband_btn.contains(x, y))
        {
            if (on_disband_) on_disband_();
            return true;
        }
    }
    else
    {
        ui_rect leave_btn{bx + 120, bottom_y, 90, 24};
        if (leave_btn.contains(x, y))
        {
            if (on_leave_) on_leave_();
            return true;
        }
    }

    active_field_ = text_field::none;
    return false;
}

bool guild_dialog::handle_key_press(sf::Keyboard::Key key)
{
    if (!visible_ || active_field_ == text_field::none) return false;

    if (key == sf::Keyboard::Key::Escape)
    {
        active_field_ = text_field::none;
        return true;
    }

    if (key == sf::Keyboard::Key::Tab)
    {
        if (active_field_ == text_field::create_name)
            active_field_ = text_field::create_tag;
        else if (active_field_ == text_field::create_tag)
            active_field_ = text_field::create_name;
        return true;
    }

    if (key == sf::Keyboard::Key::Enter)
    {
        if (active_field_ == text_field::create_name || active_field_ == text_field::create_tag)
        {
            if (on_create_ && !create_name_input_.empty() && !create_tag_input_.empty())
            {
                on_create_(create_name_input_, create_tag_input_);
                create_name_input_.clear();
                create_tag_input_.clear();
                active_field_ = text_field::none;
            }
        }
        else if (active_field_ == text_field::invite_target)
        {
            if (on_invite_ && !invite_input_.empty())
            {
                on_invite_(invite_input_);
                invite_input_.clear();
            }
        }
        else if (active_field_ == text_field::motd)
        {
            if (on_set_motd_ && !motd_input_.empty())
            {
                on_set_motd_(motd_input_);
                motd_input_.clear();
                active_field_ = text_field::none;
            }
        }
        return true;
    }

    if (key == sf::Keyboard::Key::Backspace)
    {
        std::string* target = nullptr;
        switch (active_field_)
        {
            case text_field::create_name: target = &create_name_input_; break;
            case text_field::create_tag: target = &create_tag_input_; break;
            case text_field::invite_target: target = &invite_input_; break;
            case text_field::motd: target = &motd_input_; break;
            default: return false;
        }
        if (target && !target->empty())
            target->pop_back();
        return true;
    }

    return false;
}

bool guild_dialog::handle_text_input(char32_t unicode)
{
    if (!visible_ || active_field_ == text_field::none) return false;

    // Only printable ASCII
    if (unicode < 32 || unicode > 126) return false;

    char ch = static_cast<char>(unicode);

    switch (active_field_)
    {
        case text_field::create_name:
            if (create_name_input_.size() < 20)
                create_name_input_ += ch;
            return true;
        case text_field::create_tag:
            if (create_tag_input_.size() < 4)
                create_tag_input_ += ch;
            return true;
        case text_field::invite_target:
            if (invite_input_.size() < 20)
                invite_input_ += ch;
            return true;
        case text_field::motd:
            if (motd_input_.size() < 120)
                motd_input_ += ch;
            return true;
        default:
            return false;
    }
}

void guild_dialog::set_members(const std::vector<member_display_info>& members) {
    members_ = members;
    scroll_offset_ = 0;
}

void guild_dialog::clear_members() {
    members_.clear();
    scroll_offset_ = 0;
}

void guild_dialog::set_pending_invite(std::string_view guild_name, std::string_view guild_tag,
                                       std::string_view inviter_name, float time_remaining)
{
    has_pending_invite_ = true;
    invite_guild_name_ = guild_name;
    invite_guild_tag_ = guild_tag;
    invite_from_ = inviter_name;
    invite_time_remaining_ = time_remaining;
}

void guild_dialog::clear_pending_invite()
{
    has_pending_invite_ = false;
}

} // namespace hb
