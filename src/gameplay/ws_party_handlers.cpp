// Party over the JSON protocol (docs/protocol/social.md): invite by name, accept or
// decline the invite notice, leave, and the party_update push that lists the members.

#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "network/messages/quest.hpp"
#include "ui/dialogs/party_dialog.hpp"
#include "entity/entity.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

namespace
{

party_dialog* get_party_dialog(game_state_manager& game)
{
    return dynamic_cast<party_dialog*>(game.ui().get_dialog(dialog_type::party));
}

std::string party_error_text(const std::string& code)
{
    if (code == "player_not_found")
        return "No player by that name is online.";
    if (code == "already_in_party")
        return "Already in a party.";
    if (code == "party_full")
        return "The party is full.";
    if (code == "not_leader")
        return "Only the party leader can invite.";
    if (code == "party_not_found")
        return "That party no longer exists.";
    if (code == "no_pending_invite")
        return "There is no invite to answer.";
    if (code == "invite_expired")
        return "The invite has expired.";
    return code.empty() ? "Party request failed." : "Party request failed: " + code;
}

} // namespace

void ws_message_handler::request_party_invite(std::string_view target_name)
{
    game_->ws_connection().send(make_party_invite_request(target_name));
    spdlog::info("Sent party_invite_request: target='{}'", target_name);
}

void ws_message_handler::request_party_accept(uint32_t party_id, bool accept)
{
    game_->ws_connection().send(make_party_accept_request(party_id, accept));
    spdlog::info("Sent party_accept_request: party={} accept={}", party_id, accept);
}

void ws_message_handler::request_party_leave()
{
    game_->ws_connection().send(make_party_leave_request());
    spdlog::info("Sent party_leave_request");
}

void ws_message_handler::handle_party_invite_response(const json& message)
{
    auto r = party_action_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event(party_error_text(r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event("Party invite sent.", message_color::green);
}

void ws_message_handler::handle_party_invite_notice(const json& message)
{
    auto r = party_invite_notice_data::from_json(message);
    spdlog::info("Party invite from '{}' (party {})", r.inviter_name, r.party_id);
    if (auto* dlg = get_party_dialog(*game_))
    {
        dlg->set_pending_invite(r.inviter_name, r.party_id);
        if (!dlg->is_open())
            game_->ui().open_dialog(dialog_type::party);
    }
    game_->get_status_log().add_event(r.inviter_name + " invites you to a party (P to answer).",
                                      message_color::green);
}

void ws_message_handler::handle_party_accept_response(const json& message)
{
    auto r = party_action_response_data::from_json(message);
    if (auto* dlg = get_party_dialog(*game_))
        dlg->clear_pending_invite();
    if (!r.success)
    {
        game_->get_status_log().add_event(party_error_text(r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event("You joined the party.", message_color::green);
}

void ws_message_handler::handle_party_leave_response(const json& message)
{
    auto r = party_action_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event(party_error_text(r.error), message_color::red);
        return;
    }
    if (auto* dlg = get_party_dialog(*game_))
        dlg->clear_members();
    game_->get_status_log().add_event("You left the party.", message_color::yellow);
}

void ws_message_handler::handle_party_update(const json& message)
{
    auto r = party_update_data::from_json(message);
    auto* dlg = get_party_dialog(*game_);
    if (!dlg)
        return;

    std::string my_name;
    if (auto* me = game_->local_player(); me && me->has_name())
        my_name = me->name().name;

    std::vector<party_member_info> members;
    uint32_t index = 1;
    for (const auto& name : r.members)
    {
        party_member_info m;
        m.entity_id = index++;
        m.name = name;
        m.is_leader = (name == r.leader_name);
        m.is_online = true;
        members.push_back(std::move(m));
    }

    const bool still_in = my_name.empty() || std::any_of(r.members.begin(), r.members.end(),
                                                         [&](const std::string& n) { return n == my_name; });
    if (members.empty() || !still_in)
    {
        dlg->clear_members();
        return;
    }
    dlg->set_members(members);

    std::string list;
    for (const auto& n : r.members)
        list += (list.empty() ? "" : ", ") + n + (n == r.leader_name ? "*" : "");
    game_->get_status_log().add_event("Party: " + list, message_color::blue);
}

} // namespace hb
