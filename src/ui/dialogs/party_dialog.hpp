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

// Forward declaration
struct guild_member_info;
struct pending_guild_invite;

// Guild dialog modes
enum class guild_dialog_mode : uint8_t
{
    no_guild = 0,       // Not in a guild - show create form
    guild_view = 1,     // In a guild - show info + members
    creating = 2,       // Creating a guild - show name/tag input
};

// Guild dialog - displays guild info and members, supports create/invite flows
class guild_dialog : public dialog {
public:
    guild_dialog();
    ~guild_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_key_press(sf::Keyboard::Key key) override;
    bool handle_text_input(char32_t unicode) override;

    // Set guild state from guild_system
    void set_mode(guild_dialog_mode mode) { mode_ = mode; }
    void set_guild_name(std::string_view name) { guild_name_ = name; }
    void set_guild_tag(std::string_view tag) { guild_tag_ = tag; }
    void set_guild_master(std::string_view name) { guild_master_ = name; }
    void set_motd(std::string_view motd) { motd_ = motd; }
    void set_local_rank(uint8_t rank) { local_rank_ = rank; }

    // Guild members list
    struct member_display_info
    {
        std::string name;
        std::string rank_name;
        uint8_t rank_id = 0;
        bool is_online = false;
    };

    void set_members(const std::vector<member_display_info>& members);
    void clear_members();

    // Pending invite
    void set_pending_invite(std::string_view guild_name, std::string_view guild_tag,
                            std::string_view inviter_name, float time_remaining);
    void clear_pending_invite();

    // Callbacks
    using action_callback = std::function<void()>;
    using name_callback = std::function<void(std::string_view)>;
    using create_callback = std::function<void(std::string_view name, std::string_view tag)>;

    void set_on_create(create_callback cb) { on_create_ = std::move(cb); }
    void set_on_leave(action_callback cb) { on_leave_ = std::move(cb); }
    void set_on_disband(action_callback cb) { on_disband_ = std::move(cb); }
    void set_on_invite(name_callback cb) { on_invite_ = std::move(cb); }
    void set_on_kick(name_callback cb) { on_kick_ = std::move(cb); }
    void set_on_promote(name_callback cb) { on_promote_ = std::move(cb); }
    void set_on_demote(name_callback cb) { on_demote_ = std::move(cb); }
    void set_on_set_motd(name_callback cb) { on_set_motd_ = std::move(cb); }
    void set_on_accept_invite(action_callback cb) { on_accept_invite_ = std::move(cb); }
    void set_on_decline_invite(action_callback cb) { on_decline_invite_ = std::move(cb); }

private:
    void render_no_guild(renderer& rend, int32_t x, int32_t y);
    void render_guild_view(renderer& rend, int32_t x, int32_t y);
    void render_invite_banner(renderer& rend, int32_t x, int32_t y);
    bool handle_click_no_guild(int32_t x, int32_t y);
    bool handle_click_guild_view(int32_t x, int32_t y);
    bool handle_click_invite_banner(int32_t x, int32_t y);

    guild_dialog_mode mode_ = guild_dialog_mode::no_guild;

    // Guild data
    std::string guild_name_;
    std::string guild_tag_;
    std::string guild_master_;
    std::string motd_;
    uint8_t local_rank_ = 255;
    std::vector<member_display_info> members_;

    // Text input state
    enum class text_field : uint8_t { none, create_name, create_tag, invite_target, motd };
    text_field active_field_ = text_field::none;
    std::string create_name_input_;
    std::string create_tag_input_;
    std::string invite_input_;
    std::string motd_input_;

    // Pending invite
    bool has_pending_invite_ = false;
    std::string invite_guild_name_;
    std::string invite_guild_tag_;
    std::string invite_from_;
    float invite_time_remaining_ = 0.0f;

    // Scroll
    int32_t scroll_offset_ = 0;
    static constexpr int32_t visible_members = 8;
    static constexpr int32_t member_row_height = 22;

    // Callbacks
    create_callback on_create_;
    action_callback on_leave_;
    action_callback on_disband_;
    name_callback on_invite_;
    name_callback on_kick_;
    name_callback on_promote_;
    name_callback on_demote_;
    name_callback on_set_motd_;
    action_callback on_accept_invite_;
    action_callback on_decline_invite_;
};

} // namespace hb
