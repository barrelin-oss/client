#pragma once

#include "network/messages/common.hpp"

namespace hb
{

// Chat message broadcast data (server -> client)
struct chat_message_broadcast_data
{
    std::string channel; // "local", "shout", "whisper", "guild", "party", "gm", "faction", "global"
    uint32_t sender_id = 0;
    std::string sender_name;
    std::string content;
    std::vector<std::string> flags; // "system", "gm", "emote", etc.
    int64_t timestamp = 0;
    std::string recipient_name; // For whispers

    static chat_message_broadcast_data from_json(const json& j)
    {
        chat_message_broadcast_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("channel"))
                data.channel = d["channel"].get<std::string>();
            if (d.contains("sender_id"))
            {
                if (d["sender_id"].is_number())
                    data.sender_id = d["sender_id"].get<uint32_t>();
                else if (d["sender_id"].is_string())
                    data.sender_id = static_cast<uint32_t>(std::stoul(d["sender_id"].get<std::string>()));
            }
            if (d.contains("sender_name"))
                data.sender_name = d["sender_name"].get<std::string>();
            if (d.contains("content"))
                data.content = d["content"].get<std::string>();
            if (d.contains("flags") && d["flags"].is_array())
            {
                for (const auto& f : d["flags"])
                {
                    data.flags.push_back(f.get<std::string>());
                }
            }
            if (d.contains("timestamp"))
            {
                if (d["timestamp"].is_number())
                    data.timestamp = d["timestamp"].get<int64_t>();
                // else: ISO 8601 string — ignore for now, we use local clock
            }
            if (d.contains("recipient_name") && d["recipient_name"].is_string())
                data.recipient_name = d["recipient_name"].get<std::string>();
        }
        return data;
    }
};

// Guild create response
struct guild_create_response_data
{
    bool success = false;
    std::string guild_name;
    std::string tag;
    std::string error;

    static guild_create_response_data from_json(const json& j)
    {
        guild_create_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("guild_name"))
                data.guild_name = d["guild_name"].get<std::string>();
            if (d.contains("tag"))
                data.tag = d["tag"].get<std::string>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Reusable for disband/leave/kick/promote/demote/motd responses
struct guild_action_response_data
{
    bool success = false;
    std::string error;

    static guild_action_response_data from_json(const json& j)
    {
        guild_action_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Server push: someone invited us
struct guild_invite_received_data
{
    std::string guild_name;
    std::string guild_tag;
    std::string inviter_name;

    static guild_invite_received_data from_json(const json& j)
    {
        guild_invite_received_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("guild_name"))
                data.guild_name = d["guild_name"].get<std::string>();
            if (d.contains("guild_tag"))
                data.guild_tag = d["guild_tag"].get<std::string>();
            if (d.contains("inviter_name"))
                data.inviter_name = d["inviter_name"].get<std::string>();
        }
        return data;
    }
};

// Accept/decline result
struct guild_invite_respond_response_data
{
    bool success = false;
    bool accepted = false;
    std::string guild_name;
    std::string guild_tag;
    std::string error;

    static guild_invite_respond_response_data from_json(const json& j)
    {
        guild_invite_respond_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("accepted"))
                data.accepted = d["accepted"].get<bool>();
            if (d.contains("guild_name"))
                data.guild_name = d["guild_name"].get<std::string>();
            if (d.contains("guild_tag"))
                data.guild_tag = d["guild_tag"].get<std::string>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Guild member in info response
struct guild_member_data
{
    std::string name;
    uint8_t rank = 0;
    std::string rank_name;
    bool is_online = false;

    static guild_member_data from_json(const json& j)
    {
        guild_member_data data;
        if (j.contains("name"))
            data.name = j["name"].get<std::string>();
        if (j.contains("rank"))
            data.rank = j["rank"].get<uint8_t>();
        if (j.contains("rank_name"))
            data.rank_name = j["rank_name"].get<std::string>();
        if (j.contains("is_online"))
            data.is_online = j["is_online"].get<bool>();
        return data;
    }
};

// Full guild info response
struct guild_info_response_data
{
    bool success = false;
    std::string guild_name;
    std::string tag;
    std::string motd;
    std::string master_name;
    int32_t member_count = 0;
    std::vector<guild_member_data> members;
    std::string error;

    static guild_info_response_data from_json(const json& j)
    {
        guild_info_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("guild_name"))
                data.guild_name = d["guild_name"].get<std::string>();
            if (d.contains("tag"))
                data.tag = d["tag"].get<std::string>();
            if (d.contains("motd"))
                data.motd = d["motd"].get<std::string>();
            if (d.contains("master_name"))
                data.master_name = d["master_name"].get<std::string>();
            if (d.contains("member_count"))
                data.member_count = d["member_count"].get<int32_t>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
            if (d.contains("members") && d["members"].is_array())
            {
                for (const auto& m : d["members"])
                {
                    data.members.push_back(guild_member_data::from_json(m));
                }
            }
        }
        return data;
    }
};

// Guild update broadcast
struct guild_update_data
{
    std::string action;
    std::string guild_name;
    std::string player_name;
    std::string motd;

    static guild_update_data from_json(const json& j)
    {
        guild_update_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("action"))
                data.action = d["action"].get<std::string>();
            if (d.contains("guild_name"))
                data.guild_name = d["guild_name"].get<std::string>();
            if (d.contains("player_name"))
                data.player_name = d["player_name"].get<std::string>();
            if (d.contains("motd"))
                data.motd = d["motd"].get<std::string>();
        }
        return data;
    }
};

// Convenience functions
inline json
make_chat_message_request(std::string_view content, std::string_view channel, std::string_view recipient = "")
{
    auto builder = message_builder(msg_type::chat_message)
                       .set("content", std::string(content))
                       .set("channel", std::string(channel));
    if (!recipient.empty())
    {
        builder.set("recipient", std::string(recipient));
    }
    return builder.build();
}

// Guild request builders
inline json make_guild_create_request(std::string_view name, std::string_view tag)
{
    return message_builder(msg_type::guild_create_request)
        .set("name", std::string(name))
        .set("tag", std::string(tag))
        .build();
}

inline json make_guild_disband_request()
{
    return message_builder(msg_type::guild_disband_request).build();
}

inline json make_guild_leave_request()
{
    return message_builder(msg_type::guild_leave_request).build();
}

inline json make_guild_kick_request(std::string_view target_name)
{
    return message_builder(msg_type::guild_kick_request).set("target_name", std::string(target_name)).build();
}

inline json make_guild_invite_request(std::string_view target_name)
{
    return message_builder(msg_type::guild_invite_request).set("target_name", std::string(target_name)).build();
}

inline json make_guild_invite_respond_request(bool accept)
{
    return message_builder(msg_type::guild_invite_respond_request).set("accept", accept).build();
}

inline json make_guild_promote_request(std::string_view target_name)
{
    return message_builder(msg_type::guild_promote_request).set("target_name", std::string(target_name)).build();
}

inline json make_guild_demote_request(std::string_view target_name)
{
    return message_builder(msg_type::guild_demote_request).set("target_name", std::string(target_name)).build();
}

inline json make_guild_set_motd_request(std::string_view motd)
{
    return message_builder(msg_type::guild_set_motd_request).set("motd", std::string(motd)).build();
}

inline json make_guild_info_request()
{
    return message_builder(msg_type::guild_info_request).build();
}

} // namespace hb
