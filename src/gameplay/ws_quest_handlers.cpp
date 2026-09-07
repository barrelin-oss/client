// NPC dialog choices and quests over the JSON protocol (docs/protocol/npc.md, quest.md).
// The dialog that an officer opens leads to open_quests, which makes the server push a
// quest_list_response; everything else here is request/response plus the quest_update push.

#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "network/messages/quest.hpp"
#include "ui/dialogs/npc_dialog.hpp"
#include "ui/dialogs/quest_dialog.hpp"
#include "entity/entity.hpp"
#include <chrono>
#include <format>
#include <spdlog/spdlog.h>

namespace hb
{

namespace
{

quest_dialog* get_quest_dialog(game_state_manager& game)
{
    return dynamic_cast<quest_dialog*>(game.ui().get_dialog(dialog_type::quest));
}

npc_dialog* get_npc_dialog(game_state_manager& game)
{
    return dynamic_cast<npc_dialog*>(game.ui().get_dialog(dialog_type::npc_dialog));
}

std::string quest_error_text(const std::string& code)
{
    if (code == "quest_not_found")
        return "That quest does not exist.";
    if (code == "wrong_npc")
        return "This officer does not handle that quest.";
    if (code == "level_out_of_range")
        return "Your level is outside the range of that quest.";
    if (code == "wrong_faction")
        return "That quest is not for your city.";
    if (code == "already_active")
        return "You already have that quest.";
    if (code == "quest_log_full")
        return "Your quest log is full.";
    if (code == "missing_prerequisite")
        return "You have not finished the quest that comes before it.";
    if (code == "objectives_incomplete")
        return "The quest is not finished yet.";
    if (code == "not_active")
        return "That quest is not active.";
    if (code == "quests_unavailable")
        return "This NPC has no tasks for you.";
    return code.empty() ? "Quest request failed." : "Quest request failed: " + code;
}

std::vector<quest_view> to_views(const std::vector<quest_data>& quests)
{
    std::vector<quest_view> views;
    views.reserve(quests.size());
    for (const auto& q : quests)
        views.push_back(quest_view::from_data(q));
    return views;
}

} // namespace

// ---------------------------------------------------------------------------
// NPC dialog
// ---------------------------------------------------------------------------

void ws_message_handler::request_interact(uint32_t target_id)
{
    auto* player = game_->local_player();
    if (!player)
        return;
    const auto& t = player->transform();
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    game_->ws_connection().send(make_player_interact_request(t.tile_x, t.tile_y, target_id, now));
    spdlog::info("Sent player_interact_request: target={}", target_id);
}

void ws_message_handler::open_npc_dialog(uint32_t npc_entity_id, const json& interaction_data)
{
    show_npc_node(npc_entity_id, npc_dialog_node_data::from_object(interaction_data));
}

void ws_message_handler::show_npc_node(uint32_t npc_entity_id, const npc_dialog_node_data& node)
{
    dialog_npc_id_ = npc_entity_id;
    dialog_node_id_ = node.node_id;
    if (!node.npc_name.empty())
        dialog_npc_name_ = node.npc_name;

    auto* dlg = get_npc_dialog(*game_);
    if (!dlg)
    {
        spdlog::warn("NPC dialog window is not registered; cannot show node '{}'", node.node_id);
        return;
    }
    dlg->set_npc_name(dialog_npc_name_);
    dlg->set_message(node.text);
    std::vector<npc_option> options;
    int32_t index = 0;
    for (const auto& o : node.options)
        options.push_back({o.label, index++, true});
    dlg->set_options(options);
    if (!dlg->is_open())
        game_->ui().open_dialog(dialog_type::npc_dialog);
    spdlog::info("NPC dialog '{}' node '{}': {} options, open={}", dialog_npc_name_, node.node_id, options.size(), dlg->is_open());
}

void ws_message_handler::request_dialog_choice(int32_t choice_index)
{
    if (dialog_npc_id_ == 0)
        return;
    game_->ws_connection().send(make_dialog_choice_request(dialog_npc_id_, dialog_node_id_, choice_index));
    spdlog::info("Sent dialog_choice_request: npc={} node='{}' choice={}", dialog_npc_id_, dialog_node_id_, choice_index);
}

void ws_message_handler::handle_dialog_choice_response(const json& message)
{
    auto r = dialog_choice_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event("The NPC turns away: " + r.error, message_color::red);
        game_->ui().close_dialog(dialog_type::npc_dialog);
        return;
    }

    if (r.action == "goto_node")
    {
        show_npc_node(dialog_npc_id_, r.node);
        return;
    }

    // Every other action ends the conversation; what follows arrives on its own
    // (quest_list_response for open_quests, quest_complete_response for claim_rewards).
    game_->ui().close_dialog(dialog_type::npc_dialog);
    if (r.action == "open_shop" || r.action == "open_bank")
    {
        // The server answers a plain interact with the shop or bank when the NPC has one
        request_interact(dialog_npc_id_);
    }
}

// ---------------------------------------------------------------------------
// Quest requests
// ---------------------------------------------------------------------------

void ws_message_handler::request_quest_list(uint32_t npc_entity_id)
{
    game_->ws_connection().send(make_quest_list_request(npc_entity_id));
}

void ws_message_handler::request_quest_accept(uint32_t npc_entity_id, uint32_t quest_id)
{
    game_->ws_connection().send(make_quest_accept_request(npc_entity_id, quest_id));
    spdlog::info("Sent quest_accept_request: npc={} quest={}", npc_entity_id, quest_id);
}

void ws_message_handler::request_quest_abandon(uint32_t quest_id)
{
    game_->ws_connection().send(make_quest_abandon_request(quest_id));
    spdlog::info("Sent quest_abandon_request: quest={}", quest_id);
}

void ws_message_handler::request_quest_complete(uint32_t npc_entity_id, uint32_t quest_id)
{
    game_->ws_connection().send(make_quest_complete_request(npc_entity_id, quest_id));
    spdlog::info("Sent quest_complete_request: npc={} quest={}", npc_entity_id, quest_id);
}

void ws_message_handler::request_quest_journal()
{
    game_->ws_connection().send(make_quest_journal_request());
}

// ---------------------------------------------------------------------------
// Quest responses and pushes
// ---------------------------------------------------------------------------

void ws_message_handler::handle_quest_list_response(const json& message)
{
    auto r = quest_list_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event(quest_error_text(r.error), message_color::red);
        return;
    }
    auto* dlg = get_quest_dialog(*game_);
    if (!dlg)
        return;
    game_->ui().close_dialog(dialog_type::npc_dialog);
    const std::string npc_name = (r.npc_entity_id == dialog_npc_id_) ? dialog_npc_name_ : std::string{};
    dlg->show_offers(r.npc_entity_id, npc_name, to_views(r.quests));
    spdlog::info("Quest list from npc {}: {} quests", r.npc_entity_id, r.quests.size());
}

void ws_message_handler::handle_quest_accept_response(const json& message)
{
    auto r = quest_action_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event(quest_error_text(r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event("Quest accepted.", message_color::green);
    // The quest_update push that follows refreshes the dialog's copy
}

void ws_message_handler::handle_quest_abandon_response(const json& message)
{
    auto r = quest_action_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event(quest_error_text(r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event("Quest abandoned.", message_color::yellow);
    if (auto* dlg = get_quest_dialog(*game_))
    {
        if (dlg->from_npc())
            request_quest_list(dlg->npc_entity_id());
        else
            dlg->remove_quest(r.quest_id);
    }
}

void ws_message_handler::handle_quest_complete_response(const json& message)
{
    auto r = quest_action_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event(quest_error_text(r.error), message_color::red);
        return;
    }
    std::string text = "Quest complete";
    if (r.reward_experience > 0 || r.reward_gold > 0)
        text += std::format(": +{} exp, +{} gold", r.reward_experience, r.reward_gold);
    game_->get_status_log().add_event(text + ".", message_color::green);
    if (auto* dlg = get_quest_dialog(*game_))
    {
        if (dlg->from_npc())
            request_quest_list(dlg->npc_entity_id());
        else
            dlg->remove_quest(r.quest_id);
    }
}

void ws_message_handler::handle_quest_journal_response(const json& message)
{
    auto r = quest_list_response_data::from_json(message);
    auto* dlg = get_quest_dialog(*game_);
    if (!dlg)
        return;
    // A journal that arrives while the officer's list is on screen must not replace it
    if (dlg->is_open() && dlg->from_npc())
        return;
    dlg->show_journal(to_views(r.quests));
}

// Specialties (monster mastery) and achievements: the server also writes a system chat line; here
// they go to the on-screen event log, in gold, so they are not missed.
void ws_message_handler::handle_specialty_update(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];
    game_->get_status_log().add_event(std::format("{} specialty reached level {} ({} kills)",
                                                  d.value("npc_name", std::string("Monster")),
                                                  d.value("level", 0),
                                                  d.value("kills", 0)),
                                      message_color::yellow);
}

void ws_message_handler::handle_achievement_unlocked(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];
    game_->get_status_log().add_event(std::format("Achievement unlocked: {} (+{} points)",
                                                  d.value("name", std::string("?")),
                                                  d.value("points", 0)),
                                      message_color::yellow);
}

void ws_message_handler::handle_quest_update(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto q = quest_data::from_object(message["data"]);
    if (auto* dlg = get_quest_dialog(*game_))
        dlg->apply_update(quest_view::from_data(q));

    if (q.status == "complete")
    {
        game_->get_status_log().add_event(std::format("Quest '{}' is complete. Report back to the officer.", q.name),
                                          message_color::green);
    }
    else if (q.status == "active")
    {
        for (const auto& o : q.objectives)
        {
            if (o.required > 0 && o.current > 0 && !o.complete)
            {
                game_->get_status_log().add_event(std::format("{}: {}/{}", o.description, o.current, o.required),
                                                  message_color::blue);
                break;
            }
        }
    }
}

} // namespace hb
