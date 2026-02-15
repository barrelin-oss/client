#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void ws_message_handler::request_guild_create(std::string_view name, std::string_view tag)
{
    game_->ws_connection().send(make_guild_create_request(name, tag));
    spdlog::info("Sent guild_create_request: name='{}' tag='{}'", name, tag);
}

void ws_message_handler::request_guild_disband()
{
    game_->ws_connection().send(make_guild_disband_request());
    spdlog::info("Sent guild_disband_request");
}

void ws_message_handler::request_guild_leave()
{
    game_->ws_connection().send(make_guild_leave_request());
    spdlog::info("Sent guild_leave_request");
}

void ws_message_handler::request_guild_kick(std::string_view target)
{
    game_->ws_connection().send(make_guild_kick_request(target));
    spdlog::info("Sent guild_kick_request: target='{}'", target);
}

void ws_message_handler::request_guild_invite(std::string_view target)
{
    game_->ws_connection().send(make_guild_invite_request(target));
    spdlog::info("Sent guild_invite_request: target='{}'", target);
}

void ws_message_handler::request_guild_invite_respond(bool accept)
{
    game_->ws_connection().send(make_guild_invite_respond_request(accept));
    spdlog::info("Sent guild_invite_respond_request: accept={}", accept);
}

void ws_message_handler::request_guild_promote(std::string_view target)
{
    game_->ws_connection().send(make_guild_promote_request(target));
    spdlog::info("Sent guild_promote_request: target='{}'", target);
}

void ws_message_handler::request_guild_demote(std::string_view target)
{
    game_->ws_connection().send(make_guild_demote_request(target));
    spdlog::info("Sent guild_demote_request: target='{}'", target);
}

void ws_message_handler::request_guild_set_motd(std::string_view motd)
{
    game_->ws_connection().send(make_guild_set_motd_request(motd));
    spdlog::info("Sent guild_set_motd_request");
}

void ws_message_handler::request_guild_info()
{
    game_->ws_connection().send(make_guild_info_request());
    spdlog::debug("Sent guild_info_request");
}

void ws_message_handler::handle_guild_create_response(const json& message)
{
    auto data = guild_create_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild create failed: {}", data.error);
        game_->get_status_log().add_event("Guild creation failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild created: {} [{}]", data.guild_name, data.tag);
    game_->guild().set_guild(data.guild_name, data.tag, 0);
    game_->get_status_log().add_event("Guild '" + data.guild_name + "' created!", message_color::green);

    // Update local player name component
    if (auto* player = game_->local_player())
    {
        player->name().guild_name = data.guild_name;
        player->name().guild_tag = data.tag;
        player->name().guild_rank_id = 0;
    }
}

void ws_message_handler::handle_guild_disband_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild disband failed: {}", data.error);
        game_->get_status_log().add_event("Disband failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild disbanded");
    game_->guild().clear_guild();
    game_->get_status_log().add_event("Guild has been disbanded.", message_color::yellow);

    if (auto* player = game_->local_player())
    {
        player->name().guild_name.clear();
        player->name().guild_tag.clear();
        player->name().guild_rank_id = 255;
    }
}

void ws_message_handler::handle_guild_leave_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild leave failed: {}", data.error);
        game_->get_status_log().add_event("Leave failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Left guild");
    game_->guild().clear_guild();
    game_->get_status_log().add_event("You have left the guild.", message_color::yellow);

    if (auto* player = game_->local_player())
    {
        player->name().guild_name.clear();
        player->name().guild_tag.clear();
        player->name().guild_rank_id = 255;
    }
}

void ws_message_handler::handle_guild_kick_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild kick failed: {}", data.error);
        game_->get_status_log().add_event("Kick failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Kick successful");
    game_->get_status_log().add_event("Player kicked from guild.", message_color::yellow);
}

void ws_message_handler::handle_guild_invite_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild invite failed: {}", data.error);
        game_->get_status_log().add_event("Invite failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild invite sent");
    game_->get_status_log().add_event("Guild invite sent.", message_color::green);
}

void ws_message_handler::handle_guild_invite_received(const json& message)
{
    auto data = guild_invite_received_data::from_json(message);

    spdlog::info("Received guild invite from '{}' to join [{}] {}", data.inviter_name, data.guild_tag, data.guild_name);

    game_->guild().set_pending_invite(data.guild_name, data.guild_tag, data.inviter_name);
    game_->get_status_log().add_event(
        data.inviter_name + " invites you to join [" + data.guild_tag + "] " + data.guild_name, message_color::green);
}

void ws_message_handler::handle_guild_invite_respond_response(const json& message)
{
    auto data = guild_invite_respond_response_data::from_json(message);

    game_->guild().clear_pending_invite();

    if (!data.success)
    {
        spdlog::info("Guild invite respond failed: {}", data.error);
        game_->get_status_log().add_event("Join failed: " + data.error, message_color::red);
        return;
    }

    if (data.accepted)
    {
        spdlog::info("Joined guild: {} [{}]", data.guild_name, data.guild_tag);
        game_->guild().set_guild(data.guild_name, data.guild_tag, 4); // recruit rank
        game_->get_status_log().add_event("You joined [" + data.guild_tag + "] " + data.guild_name + "!",
                                          message_color::green);

        if (auto* player = game_->local_player())
        {
            player->name().guild_name = data.guild_name;
            player->name().guild_tag = data.guild_tag;
            player->name().guild_rank_id = 4;
        }

        // Request full guild info now that we've joined
        request_guild_info();
    }
    else
    {
        spdlog::info("Declined guild invite");
        game_->get_status_log().add_event("Guild invite declined.", message_color::yellow);
    }
}

void ws_message_handler::handle_guild_promote_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild promote failed: {}", data.error);
        game_->get_status_log().add_event("Promote failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild promote successful");
    game_->get_status_log().add_event("Member promoted.", message_color::green);
}

void ws_message_handler::handle_guild_demote_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild demote failed: {}", data.error);
        game_->get_status_log().add_event("Demote failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild demote successful");
    game_->get_status_log().add_event("Member demoted.", message_color::yellow);
}

void ws_message_handler::handle_guild_set_motd_response(const json& message)
{
    auto data = guild_action_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild set MOTD failed: {}", data.error);
        game_->get_status_log().add_event("Set MOTD failed: " + data.error, message_color::red);
        return;
    }

    spdlog::info("Guild MOTD updated");
    game_->get_status_log().add_event("Guild MOTD updated.", message_color::green);
}

void ws_message_handler::handle_guild_info_response(const json& message)
{
    auto data = guild_info_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Guild info failed: {}", data.error);
        return;
    }

    std::vector<guild_member_info> members;
    members.reserve(data.members.size());
    for (const auto& m : data.members)
    {
        members.push_back({
            .name = m.name,
            .rank_id = m.rank,
            .rank_name = m.rank_name,
            .is_online = m.is_online,
        });
    }

    game_->guild().set_guild_info(data.guild_name, data.tag, data.motd, data.master_name, std::move(members));

    spdlog::info("Guild info: {} [{}] - {} members", data.guild_name, data.tag, data.member_count);
}

void ws_message_handler::handle_guild_update(const json& message)
{
    auto data = guild_update_data::from_json(message);
    auto& guild = game_->guild();

    spdlog::info("Guild update: action='{}' player='{}'", data.action, data.player_name);

    if (data.action == "member_joined")
    {
        game_->get_status_log().add_event(data.player_name + " joined the guild.", message_color::green);
        request_guild_info();
    }
    else if (data.action == "member_left")
    {
        game_->get_status_log().add_event(data.player_name + " left the guild.", message_color::yellow);
        request_guild_info();
    }
    else if (data.action == "member_kicked")
    {
        game_->get_status_log().add_event(data.player_name + " was kicked from the guild.", message_color::yellow);
        request_guild_info();
    }
    else if (data.action == "you_were_kicked")
    {
        guild.clear_guild();
        game_->get_status_log().add_event("You were kicked from the guild!", message_color::red);

        if (auto* player = game_->local_player())
        {
            player->name().guild_name.clear();
            player->name().guild_tag.clear();
            player->name().guild_rank_id = 255;
        }
    }
    else if (data.action == "guild_disbanded")
    {
        guild.clear_guild();
        game_->get_status_log().add_event("The guild has been disbanded.", message_color::red);

        if (auto* player = game_->local_player())
        {
            player->name().guild_name.clear();
            player->name().guild_tag.clear();
            player->name().guild_rank_id = 255;
        }
    }
    else if (data.action == "member_promoted")
    {
        game_->get_status_log().add_event(data.player_name + " was promoted.", message_color::green);
        request_guild_info();
    }
    else if (data.action == "member_demoted")
    {
        game_->get_status_log().add_event(data.player_name + " was demoted.", message_color::yellow);
        request_guild_info();
    }
    else if (data.action == "motd_changed")
    {
        game_->get_status_log().add_event("Guild MOTD: " + data.motd, message_color::green);
        request_guild_info();
    }
}

} // namespace hb
