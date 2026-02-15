#pragma once

#include "network/messages/common.hpp"

namespace hb
{

// Combat attack broadcast data (server -> all nearby clients)
struct combat_attack_broadcast_data
{
    uint32_t attacker_id = 0;
    uint32_t target_id = 0;
    int32_t attacker_x = 0;
    int32_t attacker_y = 0;
    int32_t target_x = 0;
    int32_t target_y = 0;
    bool hit = false;
    bool critical = false;
    int32_t damage = 0;
    std::string attack_mode;     // "melee" or "ranged"
    std::string projectile_type; // "", "arrow", or "poison_arrow"
    uint8_t direction = 0;

    bool is_ranged() const { return attack_mode == "ranged"; }

    static combat_attack_broadcast_data from_json(const json& j)
    {
        combat_attack_broadcast_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("attacker_id"))
                data.attacker_id = d["attacker_id"].get<uint32_t>();
            if (d.contains("target_id"))
                data.target_id = d["target_id"].get<uint32_t>();
            if (d.contains("attacker_x"))
                data.attacker_x = d["attacker_x"].get<int32_t>();
            if (d.contains("attacker_y"))
                data.attacker_y = d["attacker_y"].get<int32_t>();
            if (d.contains("target_x"))
                data.target_x = d["target_x"].get<int32_t>();
            if (d.contains("target_y"))
                data.target_y = d["target_y"].get<int32_t>();
            if (d.contains("hit"))
                data.hit = d["hit"].get<bool>();
            if (d.contains("critical"))
                data.critical = d["critical"].get<bool>();
            if (d.contains("damage"))
                data.damage = d["damage"].get<int32_t>();
            if (d.contains("attack_mode"))
                data.attack_mode = d["attack_mode"].get<std::string>();
            if (d.contains("projectile_type") && !d["projectile_type"].is_null())
                data.projectile_type = d["projectile_type"].get<std::string>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<uint8_t>();
        }
        return data;
    }
};

// Player attack response data (server -> attacker only)
// Server nests result fields inside data.result
struct player_attack_response_data
{
    bool success = false;
    std::string error;
    bool hit = false;
    bool critical = false;
    int32_t damage = 0;
    uint32_t target_id = 0;
    int32_t target_hp = 0;
    int32_t target_hp_max = 0;
    bool is_ranged = false;
    int32_t ammo_count = -1;       // remaining arrows (-1 = not applicable)
    uint32_t ammo_template_id = 0; // which arrow type was consumed

    static player_attack_response_data from_json(const json& j)
    {
        player_attack_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
            // Result fields are nested inside data.result
            if (d.contains("result"))
            {
                const auto& r = d["result"];
                if (r.contains("hit"))
                    data.hit = r["hit"].get<bool>();
                if (r.contains("critical"))
                    data.critical = r["critical"].get<bool>();
                if (r.contains("damage"))
                    data.damage = r["damage"].get<int32_t>();
                if (r.contains("target_id"))
                    data.target_id = r["target_id"].get<uint32_t>();
                if (r.contains("target_hp"))
                    data.target_hp = r["target_hp"].get<int32_t>();
                if (r.contains("target_hp_max"))
                    data.target_hp_max = r["target_hp_max"].get<int32_t>();
                if (r.contains("is_ranged"))
                    data.is_ranged = r["is_ranged"].get<bool>();
                if (r.contains("ammo_count"))
                    data.ammo_count = r["ammo_count"].get<int32_t>();
                if (r.contains("ammo_template_id"))
                    data.ammo_template_id = r["ammo_template_id"].get<uint32_t>();
            }
        }
        return data;
    }
};

// NPC attack data (server -> all nearby clients)
struct npc_attack_data
{
    uint32_t npc_id = 0;
    uint32_t target_id = 0;
    int32_t npc_x = 0;
    int32_t npc_y = 0;
    int32_t target_x = 0;
    int32_t target_y = 0;
    int32_t damage = 0;
    bool critical = false;
    bool is_ranged = false;
    std::string projectile_type;

    bool hit() const { return damage > 0; }

    static npc_attack_data from_json(const json& j)
    {
        npc_attack_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            // Server sends "attacker_id" for npc attacks
            if (d.contains("attacker_id"))
                data.npc_id = d["attacker_id"].get<uint32_t>();
            if (d.contains("target_id"))
                data.target_id = d["target_id"].get<uint32_t>();
            if (d.contains("attacker_x"))
                data.npc_x = d["attacker_x"].get<int32_t>();
            if (d.contains("attacker_y"))
                data.npc_y = d["attacker_y"].get<int32_t>();
            if (d.contains("target_x"))
                data.target_x = d["target_x"].get<int32_t>();
            if (d.contains("target_y"))
                data.target_y = d["target_y"].get<int32_t>();
            if (d.contains("damage"))
                data.damage = d["damage"].get<int32_t>();
            // Server sends "is_critical" not "critical"
            if (d.contains("is_critical"))
                data.critical = d["is_critical"].get<bool>();
            if (d.contains("is_ranged"))
                data.is_ranged = d["is_ranged"].get<bool>();
            if (d.contains("projectile_type") && !d["projectile_type"].is_null())
                data.projectile_type = d["projectile_type"].get<std::string>();
        }
        return data;
    }
};

// Entity death data (server -> all nearby clients)
struct entity_death_data
{
    uint32_t victim_id = 0;
    uint32_t killer_id = 0;
    int32_t x = 0;
    int32_t y = 0;

    static entity_death_data from_json(const json& j)
    {
        entity_death_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("victim_id"))
                data.victim_id = d["victim_id"].get<uint32_t>();
            if (d.contains("killer_id"))
                data.killer_id = d["killer_id"].get<uint32_t>();
            if (d.contains("x"))
                data.x = d["x"].get<int32_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int32_t>();
        }
        return data;
    }
};

// Combat effect data (server -> all nearby clients)
struct combat_effect_data
{
    uint32_t source_id = 0;
    uint32_t target_id = 0;
    std::string effect_type; // "damage", "heal", "miss", "dodge", "block", "resist", "buff", "debuff"
    int32_t value = 0;
    std::string damage_type; // "physical", "fire", "ice", etc.
    uint16_t spell_id = 0;
    bool is_critical = false;
    int32_t target_x = 0;
    int32_t target_y = 0;

    static combat_effect_data from_json(const json& j)
    {
        combat_effect_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("source_id"))
                data.source_id = d["source_id"].get<uint32_t>();
            if (d.contains("target_id"))
                data.target_id = d["target_id"].get<uint32_t>();
            if (d.contains("effect_type"))
                data.effect_type = d["effect_type"].get<std::string>();
            if (d.contains("value"))
                data.value = d["value"].get<int32_t>();
            if (d.contains("damage_type"))
                data.damage_type = d["damage_type"].get<std::string>();
            if (d.contains("spell_id"))
                data.spell_id = d["spell_id"].get<uint16_t>();
            if (d.contains("is_critical"))
                data.is_critical = d["is_critical"].get<bool>();
            if (d.contains("target_x"))
                data.target_x = d["target_x"].get<int32_t>();
            if (d.contains("target_y"))
                data.target_y = d["target_y"].get<int32_t>();
        }
        return data;
    }
};

// Player death info data (server -> dead player)
struct player_death_info_data
{
    uint32_t killer_id = 0;
    std::string killer_name;
    bool is_pvp = false;
    int32_t xp_lost = 0;
    int32_t pk_points_change = 0;
    int32_t gold_reward = 0;
    int32_t respawn_delay_ms = 5000;
    std::string respawn_map;
    int32_t respawn_x = 0;
    int32_t respawn_y = 0;

    static player_death_info_data from_json(const json& j)
    {
        player_death_info_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("killer_id"))
                data.killer_id = d["killer_id"].get<uint32_t>();
            if (d.contains("killer_name"))
                data.killer_name = d["killer_name"].get<std::string>();
            if (d.contains("is_pvp"))
                data.is_pvp = d["is_pvp"].get<bool>();
            if (d.contains("xp_lost"))
                data.xp_lost = d["xp_lost"].get<int32_t>();
            if (d.contains("pk_points_change"))
                data.pk_points_change = d["pk_points_change"].get<int32_t>();
            if (d.contains("gold_reward"))
                data.gold_reward = d["gold_reward"].get<int32_t>();
            if (d.contains("respawn_delay_ms"))
                data.respawn_delay_ms = d["respawn_delay_ms"].get<int32_t>();
            if (d.contains("respawn_map"))
                data.respawn_map = d["respawn_map"].get<std::string>();
            if (d.contains("respawn_x"))
                data.respawn_x = d["respawn_x"].get<int32_t>();
            if (d.contains("respawn_y"))
                data.respawn_y = d["respawn_y"].get<int32_t>();
        }
        return data;
    }
};

// Combat mode change broadcast data
struct combat_mode_change_broadcast_data
{
    uint32_t entity_id = 0;
    bool combat_mode = false;

    static combat_mode_change_broadcast_data from_json(const json& j)
    {
        combat_mode_change_broadcast_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("combat_mode"))
                data.combat_mode = d["combat_mode"].get<bool>();
        }
        return data;
    }
};

// Combat mode change response data
struct combat_mode_change_response_data
{
    bool combat_mode = false;

    static combat_mode_change_response_data from_json(const json& j)
    {
        combat_mode_change_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("combat_mode"))
                data.combat_mode = d["combat_mode"].get<bool>();
        }
        return data;
    }
};

// Player action broadcast data
struct player_action_broadcast_data
{
    uint32_t entity_id = 0;
    std::string action;
    int16_t direction = 0;
    uint32_t target_id = 0;
    uint32_t spell_id = 0;

    static player_action_broadcast_data from_json(const json& j)
    {
        player_action_broadcast_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("action"))
                data.action = d["action"].get<std::string>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<int16_t>();
            if (d.contains("target_id"))
                data.target_id = d["target_id"].get<uint32_t>();
            if (d.contains("spell_id"))
                data.spell_id = d["spell_id"].get<uint32_t>();
        }
        return data;
    }
};

// Player magic response data (server -> caster)
// Server nests result fields inside data.result
struct player_magic_response_data
{
    bool success = false;
    std::string error;
    uint16_t spell_id = 0;
    int32_t mana_cost = 0;
    int32_t damage = 0;
    int32_t heal = 0;
    uint32_t target_id = 0;
    int32_t caster_mp = 0;

    static player_magic_response_data from_json(const json& j)
    {
        player_magic_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
            if (d.contains("result"))
            {
                const auto& r = d["result"];
                if (r.contains("spell_id"))
                    data.spell_id = r["spell_id"].get<uint16_t>();
                if (r.contains("mana_cost"))
                    data.mana_cost = r["mana_cost"].get<int32_t>();
                if (r.contains("damage"))
                    data.damage = r["damage"].get<int32_t>();
                if (r.contains("heal"))
                    data.heal = r["heal"].get<int32_t>();
                if (r.contains("target_id"))
                    data.target_id = r["target_id"].get<uint32_t>();
                if (r.contains("caster_mp"))
                    data.caster_mp = r["caster_mp"].get<int32_t>();
            }
        }
        return data;
    }
};

// Convenience functions
inline json make_combat_mode_change_request()
{
    return message_builder(msg_type::combat_mode_change_request).build();
}

inline json make_player_attack_request(uint32_t target_id,
                                       uint8_t target_type,
                                       int32_t player_x,
                                       int32_t player_y,
                                       uint8_t attack_type = 0,
                                       uint8_t direction = 0)
{
    auto builder = message_builder(msg_type::player_attack_request)
                       .set("target_id", target_id)
                       .set("target_type", target_type)
                       .set("x", player_x)
                       .set("y", player_y)
                       .set("attack_type", attack_type)
                       .set("timestamp",
                            static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                     std::chrono::system_clock::now().time_since_epoch())
                                                     .count()));
    if (direction > 0)
        builder.set("direction", direction);
    return builder.build();
}

inline json make_player_magic_request(int32_t x,
                                      int32_t y,
                                      uint8_t direction,
                                      uint16_t spell_id,
                                      const std::string& target_type,
                                      uint32_t target_id,
                                      int32_t target_x,
                                      int32_t target_y)
{
    return message_builder(msg_type::player_magic_request)
        .set("x", x)
        .set("y", y)
        .set("direction", direction)
        .set("spell_id", spell_id)
        .set("target_type", target_type)
        .set("target_id", target_id)
        .set("target_x", target_x)
        .set("target_y", target_y)
        .set("timestamp",
             static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::system_clock::now().time_since_epoch())
                                      .count()))
        .build();
}

inline json make_player_respawn_request()
{
    return message_builder(msg_type::player_respawn_request).build();
}

} // namespace hb
