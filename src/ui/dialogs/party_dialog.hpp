#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <vector>
#include <string>

namespace hb {

// Party member info
struct party_member_info {
    uint32_t entity_id = 0;
    std::string name;
    uint16_t level = 1;
    int32_t hp = 100;
    int32_t max_hp = 100;
    int32_t mp = 50;
    int32_t max_mp = 50;
    bool is_leader = false;
    bool is_online = true;
};

// Party dialog - manages party members and interactions
class party_dialog : public dialog {
public:
    static constexpr int32_t max_party_size = 8;

    party_dialog();
    ~party_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set party data
    void set_members(const std::vector<party_member_info>& members);
    void update_member(const party_member_info& member);
    void remove_member(uint32_t entity_id);
    void clear_members();

    // Set local player ID (to know if we're the leader)
    void set_local_player_id(uint32_t id) { local_player_id_ = id; }

    // Callbacks
    using member_callback = std::function<void(uint32_t entity_id)>;
    using invite_callback = std::function<void(std::string_view name)>;

    void set_on_kick(member_callback callback) { on_kick_ = std::move(callback); }
    void set_on_promote(member_callback callback) { on_promote_ = std::move(callback); }
    void set_on_leave(std::function<void()> callback) { on_leave_ = std::move(callback); }
    void set_on_invite(invite_callback callback) { on_invite_ = std::move(callback); }

private:
    void render_member_row(renderer& rend, const party_member_info& member, int32_t y, bool hovered);
    std::optional<size_t> member_index_at(int32_t x, int32_t y) const;

    std::vector<party_member_info> members_;
    uint32_t local_player_id_ = 0;
    std::optional<size_t> hovered_index_;
    std::optional<size_t> selected_index_;

    member_callback on_kick_;
    member_callback on_promote_;
    std::function<void()> on_leave_;
    invite_callback on_invite_;

    static constexpr int32_t member_row_height = 50;
    int32_t content_start_y_ = 0;
};

// Guild dialog - displays guild info and members
class guild_dialog : public dialog {
public:
    guild_dialog();
    ~guild_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    // Guild info
    void set_guild_name(std::string_view name) { guild_name_ = name; }
    void set_guild_master(std::string_view name) { guild_master_ = name; }
    void set_member_count(int32_t count, int32_t max) { member_count_ = count; max_members_ = max; }
    void set_guild_gold(uint32_t gold) { guild_gold_ = gold; }

    // Guild members list
    struct guild_member_info {
        std::string name;
        std::string rank;
        uint16_t level = 1;
        bool is_online = false;
    };

    void set_members(const std::vector<guild_member_info>& members);
    void clear_members();

    // Callbacks
    using action_callback = std::function<void()>;
    void set_on_leave_guild(action_callback callback) { on_leave_guild_ = std::move(callback); }
    void set_on_open_bank(action_callback callback) { on_open_bank_ = std::move(callback); }

private:
    std::string guild_name_;
    std::string guild_master_;
    int32_t member_count_ = 0;
    int32_t max_members_ = 100;
    uint32_t guild_gold_ = 0;
    std::vector<guild_member_info> members_;

    int32_t scroll_offset_ = 0;
    static constexpr int32_t visible_members = 8;
    static constexpr int32_t member_row_height = 22;

    action_callback on_leave_guild_;
    action_callback on_open_bank_;
};

} // namespace hb
