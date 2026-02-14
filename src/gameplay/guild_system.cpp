#include "gameplay/guild_system.hpp"

namespace hb {

void guild_system::clear()
{
    guild_name_.clear();
    guild_tag_.clear();
    rank_ = 255;
    motd_.clear();
    master_name_.clear();
    members_.clear();
    pending_invite_.reset();
}

void guild_system::update(float dt)
{
    if (pending_invite_)
    {
        pending_invite_->time_remaining -= dt;
        if (pending_invite_->time_remaining <= 0.0f)
        {
            pending_invite_.reset();
            notify();
        }
    }
}

void guild_system::set_guild(std::string_view name, std::string_view tag, uint8_t rank)
{
    guild_name_ = name;
    guild_tag_ = tag;
    rank_ = rank;
    notify();
}

void guild_system::clear_guild()
{
    guild_name_.clear();
    guild_tag_.clear();
    rank_ = 255;
    motd_.clear();
    master_name_.clear();
    members_.clear();
    notify();
}

void guild_system::set_guild_info(std::string_view name, std::string_view tag,
                                   std::string_view motd, std::string_view master_name,
                                   std::vector<guild_member_info> members)
{
    guild_name_ = name;
    guild_tag_ = tag;
    motd_ = motd;
    master_name_ = master_name;
    members_ = std::move(members);
    notify();
}

void guild_system::set_pending_invite(std::string_view guild_name, std::string_view guild_tag,
                                       std::string_view inviter_name)
{
    pending_invite_ = pending_guild_invite{
        .guild_name = std::string(guild_name),
        .guild_tag = std::string(guild_tag),
        .inviter_name = std::string(inviter_name),
        .time_remaining = 60.0f,
    };
    notify();
}

void guild_system::clear_pending_invite()
{
    pending_invite_.reset();
    notify();
}

void guild_system::notify()
{
    if (on_change_)
        on_change_();
}

} // namespace hb
