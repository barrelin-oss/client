#pragma once

// NPC dialog choices, quests and party over the JSON protocol.
// Server specs: docs/protocol/npc.md (dialog), docs/protocol/quest.md, docs/protocol/social.md.

#include "network/messages/common.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hb
{

// A field that is documented as a string but may arrive as a number (an enum written raw)
inline std::string json_text(const json& j, const char* key)
{
    if (!j.contains(key))
        return {};
    const auto& v = j[key];
    if (v.is_string())
        return v.get<std::string>();
    if (v.is_null())
        return {};
    return v.dump();
}

// ---------------------------------------------------------------------------
// NPC dialog (player_interact_response interaction_type "dialog", dialog_choice_response)
// ---------------------------------------------------------------------------

struct npc_dialog_option_data
{
    std::string label;
    std::string action; // goto_node, close, open_shop, open_bank, open_quests, claim_rewards, ...
    std::string next_node;
};

struct npc_dialog_node_data
{
    std::string npc_name;
    std::string node_id;
    std::string text;
    std::vector<npc_dialog_option_data> options;

    // Parses the interaction_data object of player_interact_response, or the data object
    // of a goto_node dialog_choice_response (same keys).
    static npc_dialog_node_data from_object(const json& d)
    {
        npc_dialog_node_data n;
        n.npc_name = json_text(d, "npc_name");
        n.node_id = json_text(d, "node_id");
        n.text = json_text(d, "text");
        if (d.contains("options") && d["options"].is_array())
        {
            for (const auto& o : d["options"])
            {
                n.options.push_back({json_text(o, "label"), json_text(o, "action"), json_text(o, "next_node")});
            }
        }
        return n;
    }
};

struct dialog_choice_response_data
{
    bool success = false;
    std::string action;
    std::string error;
    npc_dialog_node_data node; // filled for goto_node

    static dialog_choice_response_data from_json(const json& j)
    {
        dialog_choice_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", false);
        r.action = json_text(d, "action");
        r.error = json_text(d, "error");
        if (r.action == "goto_node")
            r.node = npc_dialog_node_data::from_object(d);
        return r;
    }
};

inline json make_player_interact_request(int32_t x, int32_t y, uint32_t target_id, int64_t timestamp_ms)
{
    return message_builder(msg_type::player_interact_request)
        .set("x", x)
        .set("y", y)
        .set("target_type", std::string("npc"))
        .set("target_id", target_id)
        .set("timestamp", timestamp_ms)
        .build();
}

inline json make_dialog_choice_request(uint32_t npc_entity_id, std::string_view node_id, int32_t choice_index)
{
    return message_builder(msg_type::dialog_choice_request)
        .set("npc_entity_id", npc_entity_id)
        .set("node_id", std::string(node_id))
        .set("choice_index", choice_index)
        .build();
}

// ---------------------------------------------------------------------------
// Quests
// ---------------------------------------------------------------------------

struct quest_objective_data
{
    std::string description;
    std::string type; // kill, kill_player, collect, deliver, visit, talk, other
    std::string target_name;
    int32_t current = 0;
    int32_t required = 0;
    bool complete = false;
};

struct quest_data
{
    uint32_t quest_id = 0;
    std::string name;
    std::string description;
    int32_t min_level = 0;
    int32_t max_level = 0;
    bool repeatable = false;
    uint32_t giver_npc_id = 0;
    std::string status; // available, active, complete, turned_in, failed, abandoned
    std::vector<quest_objective_data> objectives;
    uint32_t reward_experience = 0;
    uint32_t reward_gold = 0;
    std::vector<std::string> reward_items;

    static quest_data from_object(const json& q)
    {
        quest_data d;
        d.quest_id = q.value("quest_id", 0u);
        d.name = json_text(q, "name");
        d.description = json_text(q, "description");
        d.min_level = q.value("min_level", 0);
        d.max_level = q.value("max_level", 0);
        d.repeatable = q.value("repeatable", false);
        d.giver_npc_id = q.value("giver_npc_id", 0u);
        d.status = json_text(q, "status");
        if (q.contains("objectives") && q["objectives"].is_array())
        {
            for (const auto& o : q["objectives"])
            {
                d.objectives.push_back({json_text(o, "description"), json_text(o, "type"), json_text(o, "target_name"),
                                        o.value("current", 0), o.value("required", 0), o.value("complete", false)});
            }
        }
        if (q.contains("rewards") && q["rewards"].is_object())
        {
            const auto& r = q["rewards"];
            d.reward_experience = r.value("experience", 0u);
            d.reward_gold = r.value("gold", 0u);
            if (r.contains("items") && r["items"].is_array())
            {
                for (const auto& it : r["items"])
                {
                    if (it.is_string())
                        d.reward_items.push_back(it.get<std::string>());
                    else if (it.is_object())
                        d.reward_items.push_back(it.value("name", std::string("item")) +
                                                 (it.value("count", 1) > 1 ? " x" + std::to_string(it.value("count", 1)) : ""));
                }
            }
        }
        return d;
    }
};

// quest_list_response and quest_journal_response share this shape
struct quest_list_response_data
{
    bool success = true;
    std::string error;
    uint32_t npc_entity_id = 0;
    std::vector<quest_data> quests;

    static quest_list_response_data from_json(const json& j)
    {
        quest_list_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", true);
        r.error = json_text(d, "error");
        r.npc_entity_id = d.value("npc_entity_id", 0u);
        if (d.contains("quests") && d["quests"].is_array())
        {
            for (const auto& q : d["quests"])
                r.quests.push_back(quest_data::from_object(q));
        }
        return r;
    }
};

// quest_accept_response, quest_abandon_response, quest_complete_response
struct quest_action_response_data
{
    bool success = false;
    std::string error;
    uint32_t quest_id = 0;
    uint32_t reward_experience = 0; // complete only
    uint32_t reward_gold = 0;

    static quest_action_response_data from_json(const json& j)
    {
        quest_action_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", false);
        r.error = json_text(d, "error");
        r.quest_id = d.value("quest_id", 0u);
        if (d.contains("rewards") && d["rewards"].is_object())
        {
            r.reward_experience = d["rewards"].value("experience", 0u);
            r.reward_gold = d["rewards"].value("gold", 0u);
        }
        return r;
    }
};

inline json make_quest_list_request(uint32_t npc_entity_id)
{
    return message_builder(msg_type::quest_list_request).set("npc_entity_id", npc_entity_id).build();
}

inline json make_quest_accept_request(uint32_t npc_entity_id, uint32_t quest_id)
{
    return message_builder(msg_type::quest_accept_request)
        .set("npc_entity_id", npc_entity_id)
        .set("quest_id", quest_id)
        .build();
}

inline json make_quest_abandon_request(uint32_t quest_id)
{
    return message_builder(msg_type::quest_abandon_request).set("quest_id", quest_id).build();
}

inline json make_quest_complete_request(uint32_t npc_entity_id, uint32_t quest_id)
{
    return message_builder(msg_type::quest_complete_request)
        .set("npc_entity_id", npc_entity_id)
        .set("quest_id", quest_id)
        .build();
}

inline json make_quest_journal_request()
{
    return message_builder(msg_type::quest_journal_request).build();
}

// ---------------------------------------------------------------------------
// Party
// ---------------------------------------------------------------------------

// party_invite_response, party_accept_response, party_leave_response
struct party_action_response_data
{
    bool success = false;
    std::string error;
    uint32_t party_id = 0;

    static party_action_response_data from_json(const json& j)
    {
        party_action_response_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.success = d.value("success", false);
        r.error = json_text(d, "error");
        r.party_id = d.value("party_id", 0u);
        return r;
    }
};

struct party_invite_notice_data
{
    uint32_t party_id = 0;
    std::string inviter_name;

    static party_invite_notice_data from_json(const json& j)
    {
        party_invite_notice_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.party_id = d.value("party_id", 0u);
        r.inviter_name = json_text(d, "inviter_name");
        return r;
    }
};

struct party_update_data
{
    uint32_t party_id = 0;
    std::string leader_name;
    std::vector<std::string> members;

    static party_update_data from_json(const json& j)
    {
        party_update_data r;
        if (!j.contains("data"))
            return r;
        const auto& d = j["data"];
        r.party_id = d.value("party_id", 0u);
        r.leader_name = json_text(d, "leader_name");
        if (d.contains("members") && d["members"].is_array())
        {
            for (const auto& m : d["members"])
            {
                if (m.is_string())
                    r.members.push_back(m.get<std::string>());
                else if (m.is_object())
                    r.members.push_back(json_text(m, "name"));
            }
        }
        return r;
    }
};

inline json make_party_invite_request(std::string_view target_name)
{
    return message_builder(msg_type::party_invite_request).set("target_name", std::string(target_name)).build();
}

inline json make_party_accept_request(uint32_t party_id, bool accept)
{
    return message_builder(msg_type::party_accept_request).set("party_id", party_id).set("accept", accept).build();
}

inline json make_party_leave_request()
{
    return message_builder(msg_type::party_leave_request).build();
}

} // namespace hb
