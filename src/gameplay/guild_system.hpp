#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hb {

// Guild member info (stored in guild_system after guild_info_response)
struct guild_member_info
{
    std::string name;
    uint8_t rank_id = 0;
    std::string rank_name;
    bool is_online = false;
};

// Pending guild invite from another player
struct pending_guild_invite
{
    std::string guild_name;
    std::string guild_tag;
    std::string inviter_name;
    float time_remaining = 60.0f;   // seconds until auto-expire
};

class guild_system
{
public:
    guild_system() = default;

    void clear();
    void update(float dt);

    // --- Queries ---
    bool in_guild() const { return rank_ < 255; }
    bool is_master() const { return rank_ == 0; }
    const std::string& guild_name() const { return guild_name_; }
    const std::string& guild_tag() const { return guild_tag_; }
    uint8_t rank() const { return rank_; }
    const std::string& motd() const { return motd_; }
    const std::string& master_name() const { return master_name_; }
    const std::vector<guild_member_info>& members() const { return members_; }

    bool has_pending_invite() const { return pending_invite_.has_value(); }
    const pending_guild_invite& pending_invite() const { return *pending_invite_; }

    // --- Mutators (called by ws_message_handler) ---
    void set_guild(std::string_view name, std::string_view tag, uint8_t rank);
    void clear_guild();
    void set_guild_info(std::string_view name, std::string_view tag,
                        std::string_view motd, std::string_view master_name,
                        std::vector<guild_member_info> members);
    void set_pending_invite(std::string_view guild_name, std::string_view guild_tag,
                            std::string_view inviter_name);
    void clear_pending_invite();

    // --- Callback ---
    void set_on_change(std::function<void()> cb) { on_change_ = std::move(cb); }

private:
    void notify();

    std::string guild_name_;
    std::string guild_tag_;
    uint8_t rank_ = 255;            // 255 = not in guild, 0 = master, 4 = recruit
    std::string motd_;
    std::string master_name_;
    std::vector<guild_member_info> members_;

    std::optional<pending_guild_invite> pending_invite_;

    std::function<void()> on_change_;
};

} // namespace hb
