#pragma once

#include "network/messages/common.hpp"

namespace hb
{

// Visible entity from enter_game_response world data
struct enter_game_visible_entity
{
    uint32_t entity_id = 0;
    std::string type; // "player" or "npc"
    std::string name;
    int16_t x = 0;
    int16_t y = 0;
    int16_t hp_percent = 100;
    int16_t direction = 4;    // Facing direction (0-7: N,NE,E,SE,S,SW,W,NW)
    int16_t sprite_id = 0;    // Legacy sprite type for rendering (10=Slime, etc.)
    uint32_t template_id = 0; // NPC template ID
    int16_t level = 0;        // NPC level
    std::string faction;
    std::string hostility;
    std::string pk_status;
    std::string category;
    std::vector<std::string> attributes; // NPC attributes (e.g. "Berserk", "Clairvoyant")

    static enter_game_visible_entity from_json(const json& j)
    {
        enter_game_visible_entity ent;
        if (j.contains("entity_id"))
            ent.entity_id = j["entity_id"].get<uint32_t>();
        if (j.contains("type"))
            ent.type = j["type"].get<std::string>();
        if (j.contains("name"))
            ent.name = j["name"].get<std::string>();
        if (j.contains("x"))
            ent.x = j["x"].get<int16_t>();
        if (j.contains("y"))
            ent.y = j["y"].get<int16_t>();
        if (j.contains("hp_percent"))
            ent.hp_percent = j["hp_percent"].get<int16_t>();
        if (j.contains("direction"))
            ent.direction = j["direction"].get<int16_t>();
        if (j.contains("sprite_id"))
            ent.sprite_id = j["sprite_id"].get<int16_t>();
        if (j.contains("template_id"))
            ent.template_id = j["template_id"].get<uint32_t>();
        if (j.contains("level"))
            ent.level = j["level"].get<int16_t>();
        if (j.contains("faction"))
            ent.faction = j["faction"].get<std::string>();
        if (j.contains("hostility"))
            ent.hostility = j["hostility"].get<std::string>();
        if (j.contains("pk_status"))
            ent.pk_status = j["pk_status"].get<std::string>();
        if (j.contains("category"))
            ent.category = j["category"].get<std::string>();
        if (j.contains("attributes") && j["attributes"].is_array())
        {
            for (const auto& a : j["attributes"])
                ent.attributes.push_back(a.get<std::string>());
        }
        return ent;
    }
};

// Player move response data (movement confirmation)
struct player_move_response_data
{
    bool success = false;
    int32_t x = 0;
    int32_t y = 0;
    uint8_t direction = 0;
    std::string error;

    static player_move_response_data from_json(const json& j)
    {
        player_move_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("x"))
                data.x = d["x"].get<int32_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int32_t>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<uint8_t>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Player position update broadcast data
struct player_position_update_data
{
    uint32_t entity_id = 0;
    int32_t x = 0;
    int32_t y = 0;
    uint8_t direction = 0;
    bool is_running = false;
    int32_t dest_x = -1; // Movement destination (-1 if not provided)
    int32_t dest_y = -1;

    static player_position_update_data from_json(const json& j)
    {
        player_position_update_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("x"))
                data.x = d["x"].get<int32_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int32_t>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<uint8_t>();
            if (d.contains("is_running"))
                data.is_running = d["is_running"].get<bool>();
            if (d.contains("dest_x"))
                data.dest_x = d["dest_x"].get<int32_t>();
            if (d.contains("dest_y"))
                data.dest_y = d["dest_y"].get<int32_t>();
        }
        return data;
    }
};

// Player stop response data (direction change confirmation)
struct player_stop_response_data
{
    bool success = false;
    int32_t x = 0;
    int32_t y = 0;
    uint8_t direction = 0;

    static player_stop_response_data from_json(const json& j)
    {
        player_stop_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("x"))
                data.x = d["x"].get<int32_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int32_t>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<uint8_t>();
        }
        return data;
    }
};

// Hunger update broadcast data
struct hunger_update_data
{
    int8_t level = 100;       // Hunger level (0-100)
    bool is_starving = false; // True if level <= 0

    static hunger_update_data from_json(const json& j)
    {
        hunger_update_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("level"))
                data.level = d["level"].get<int8_t>();
            if (d.contains("is_starving"))
                data.is_starving = d["is_starving"].get<bool>();
        }
        return data;
    }
};

// Stat update data
struct stat_update_data
{
    int32_t max_hp = 0;
    int32_t max_mp = 0;
    int32_t max_sp = 0;
    int32_t attack_power = 0;
    int32_t magic_power = 0;
    int32_t defense = 0;
    int32_t magic_defense = 0;
    int32_t hit_rate = 0;
    int32_t dodge_rate = 0;
    int32_t critical_rate = 0;

    // Optional vitals (included in full stat updates after teleport/respawn)
    std::optional<int32_t> hp;
    std::optional<int32_t> mp;
    std::optional<int32_t> sp;
    std::optional<int64_t> experience;
    std::optional<int32_t> gold;
    std::optional<uint8_t> level;
    std::optional<int32_t> pk_count;
    std::optional<uint8_t> hunger_level;

    static stat_update_data from_json(const json& j)
    {
        stat_update_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("max_hp"))
                data.max_hp = d["max_hp"].get<int32_t>();
            if (d.contains("max_mp"))
                data.max_mp = d["max_mp"].get<int32_t>();
            if (d.contains("max_sp"))
                data.max_sp = d["max_sp"].get<int32_t>();
            if (d.contains("attack_power"))
                data.attack_power = d["attack_power"].get<int32_t>();
            if (d.contains("magic_power"))
                data.magic_power = d["magic_power"].get<int32_t>();
            if (d.contains("defense"))
                data.defense = d["defense"].get<int32_t>();
            if (d.contains("magic_defense"))
                data.magic_defense = d["magic_defense"].get<int32_t>();
            if (d.contains("hit_rate"))
                data.hit_rate = d["hit_rate"].get<int32_t>();
            if (d.contains("dodge_rate"))
                data.dodge_rate = d["dodge_rate"].get<int32_t>();
            if (d.contains("critical_rate"))
                data.critical_rate = d["critical_rate"].get<int32_t>();
            if (d.contains("hp"))
                data.hp = d["hp"].get<int32_t>();
            if (d.contains("mp"))
                data.mp = d["mp"].get<int32_t>();
            if (d.contains("sp"))
                data.sp = d["sp"].get<int32_t>();
            if (d.contains("experience"))
                data.experience = d["experience"].get<int64_t>();
            if (d.contains("gold"))
                data.gold = d["gold"].get<int32_t>();
            if (d.contains("level"))
                data.level = d["level"].get<uint8_t>();
            if (d.contains("pk_count"))
                data.pk_count = d["pk_count"].get<int32_t>();
            if (d.contains("hunger_level"))
                data.hunger_level = d["hunger_level"].get<uint8_t>();
        }
        return data;
    }
};

// Entity HP update data
struct entity_hp_update_data
{
    uint32_t entity_id = 0;
    int32_t hp = 0;
    int32_t hp_max = 0;

    static entity_hp_update_data from_json(const json& j)
    {
        entity_hp_update_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("hp"))
                data.hp = d["hp"].get<int32_t>();
            if (d.contains("hp_max"))
                data.hp_max = d["hp_max"].get<int32_t>();
        }
        return data;
    }
};

// NPC move broadcast data
struct npc_move_data
{
    uint32_t entity_id = 0;
    int16_t x = 0;
    int16_t y = 0;
    uint8_t direction = 0;

    static npc_move_data from_json(const json& j)
    {
        npc_move_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("x"))
                data.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int16_t>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<uint8_t>();
        }
        return data;
    }
};

// NPC death data
struct npc_death_data
{
    uint32_t entity_id = 0;
    uint32_t killer_id = 0;
    int16_t x = 0;
    int16_t y = 0;

    static npc_death_data from_json(const json& j)
    {
        npc_death_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("killer_id"))
                data.killer_id = d["killer_id"].get<uint32_t>();
            if (d.contains("x"))
                data.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int16_t>();
        }
        return data;
    }
};

// Entity info response data (for both players and NPCs)
struct entity_info_response_data
{
    bool success = false;
    std::string error;

    // Common fields
    uint32_t entity_id = 0;
    std::string entity_type; // "player" or "npc"
    std::string name;
    int16_t level = 1;
    int32_t hp = 0;
    int32_t hp_max = 100;
    int16_t x = 0;
    int16_t y = 0;
    uint8_t direction = 0;

    // Common
    std::string hostility; // "friendly", "neutral", "enemy"

    // Player-specific fields
    std::string faction;    // "aresden", "elvine", etc.
    int16_t class_type = 0; // 0=Warrior, 1=Mage, etc.
    int32_t pk_count = 0;
    std::string guild_name;

    // NPC-specific fields
    uint32_t template_id = 0;
    int16_t sprite_id = 0; // Legacy sprite type for rendering (10=Slime, etc.)

    static entity_info_response_data from_json(const json& j)
    {
        entity_info_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();

            if (data.success && d.contains("entity"))
            {
                const auto& e = d["entity"];
                if (e.contains("entity_id"))
                    data.entity_id = e["entity_id"].get<uint32_t>();
                if (e.contains("entity_type"))
                    data.entity_type = e["entity_type"].get<std::string>();
                if (e.contains("name"))
                    data.name = e["name"].get<std::string>();
                if (e.contains("level"))
                    data.level = e["level"].get<int16_t>();
                if (e.contains("hp"))
                    data.hp = e["hp"].get<int32_t>();
                if (e.contains("hp_max"))
                    data.hp_max = e["hp_max"].get<int32_t>();
                if (e.contains("x"))
                    data.x = e["x"].get<int16_t>();
                if (e.contains("y"))
                    data.y = e["y"].get<int16_t>();
                if (e.contains("direction"))
                    data.direction = e["direction"].get<uint8_t>();
                if (e.contains("hostility"))
                    data.hostility = e["hostility"].get<std::string>();

                // Player-specific
                if (e.contains("faction"))
                    data.faction = e["faction"].get<std::string>();
                if (e.contains("class_type"))
                    data.class_type = e["class_type"].get<int16_t>();
                if (e.contains("pk_count"))
                    data.pk_count = e["pk_count"].get<int32_t>();
                if (e.contains("guild_name"))
                    data.guild_name = e["guild_name"].get<std::string>();

                // NPC-specific
                if (e.contains("template_id"))
                    data.template_id = e["template_id"].get<uint32_t>();
                if (e.contains("sprite_id"))
                    data.sprite_id = e["sprite_id"].get<int16_t>();
            }
        }
        return data;
    }
};

// Entity spawn data (player entered visibility range)
struct entity_spawn_data
{
    uint32_t entity_id = 0;
    std::string type; // "player", "npc", "monster"
    std::string name;
    int16_t x = 0;
    int16_t y = 0;
    int32_t hp_percent = 100;
    int16_t direction = 4; // south default
    std::string faction;
    std::string hostility;
    std::string pk_status;
    std::string guild_name;
    std::string guild_tag;
    bool combat_mode = false;
    bool is_dead = false;

    static entity_spawn_data from_json(const json& j)
    {
        entity_spawn_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("type"))
                data.type = d["type"].get<std::string>();
            if (d.contains("name"))
                data.name = d["name"].get<std::string>();
            if (d.contains("x"))
                data.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int16_t>();
            if (d.contains("hp_percent"))
                data.hp_percent = d["hp_percent"].get<int32_t>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<int16_t>();
            if (d.contains("faction"))
                data.faction = d["faction"].get<std::string>();
            if (d.contains("hostility"))
                data.hostility = d["hostility"].get<std::string>();
            if (d.contains("pk_status"))
                data.pk_status = d["pk_status"].get<std::string>();
            if (d.contains("guild_name"))
                data.guild_name = d["guild_name"].get<std::string>();
            if (d.contains("guild_tag"))
                data.guild_tag = d["guild_tag"].get<std::string>();
            if (d.contains("combat_mode"))
                data.combat_mode = d["combat_mode"].get<bool>();
            if (d.contains("is_dead"))
                data.is_dead = d["is_dead"].get<bool>();
        }
        return data;
    }
};

// NPC spawn data (NPC/monster entered visibility range)
struct npc_spawn_data
{
    uint32_t entity_id = 0;
    uint32_t template_id = 0;
    int16_t sprite_id = 0; // Legacy sprite type for rendering (10=Slime, etc.)
    std::string name;
    int16_t x = 0;
    int16_t y = 0;
    int16_t direction = 4;
    int32_t hp = 0;
    int32_t max_hp = 0;
    int16_t level = 0;
    std::string category;
    std::string hostility;
    std::vector<std::string> attributes; // e.g. "Berserk", "Clairvoyant"
    bool is_dead = false;

    static npc_spawn_data from_json(const json& j)
    {
        npc_spawn_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("template_id"))
                data.template_id = d["template_id"].get<uint32_t>();
            if (d.contains("sprite_id"))
                data.sprite_id = d["sprite_id"].get<int16_t>();
            if (d.contains("name"))
                data.name = d["name"].get<std::string>();
            if (d.contains("x"))
                data.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int16_t>();
            if (d.contains("direction"))
                data.direction = d["direction"].get<int16_t>();
            if (d.contains("hp"))
                data.hp = d["hp"].get<int32_t>();
            if (d.contains("max_hp"))
                data.max_hp = d["max_hp"].get<int32_t>();
            if (d.contains("level"))
                data.level = d["level"].get<int16_t>();
            if (d.contains("category"))
                data.category = d["category"].get<std::string>();
            if (d.contains("hostility"))
                data.hostility = d["hostility"].get<std::string>();
            if (d.contains("attributes") && d["attributes"].is_array())
            {
                for (const auto& a : d["attributes"])
                    data.attributes.push_back(a.get<std::string>());
            }
            if (d.contains("is_dead"))
                data.is_dead = d["is_dead"].get<bool>();
        }
        return data;
    }
};

// NPC despawn data
struct npc_despawn_data
{
    uint32_t entity_id = 0;

    static npc_despawn_data from_json(const json& j)
    {
        npc_despawn_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("entity_id"))
                data.entity_id = d["entity_id"].get<uint32_t>();
        }
        return data;
    }
};

// Player teleport data (server -> player, used for respawn and map transitions)
struct player_teleport_data
{
    std::string dest_map;
    int32_t dest_x = 0;
    int32_t dest_y = 0;
    uint8_t dest_dir = 4; // Default south
    std::vector<enter_game_visible_entity> entities;

    static player_teleport_data from_json(const json& j)
    {
        player_teleport_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("dest_map"))
                data.dest_map = d["dest_map"].get<std::string>();
            if (d.contains("dest_x"))
                data.dest_x = d["dest_x"].get<int32_t>();
            if (d.contains("dest_y"))
                data.dest_y = d["dest_y"].get<int32_t>();
            if (d.contains("dest_dir"))
                data.dest_dir = d["dest_dir"].get<uint8_t>();
            if (d.contains("entities") && d["entities"].is_array())
            {
                for (const auto& ent : d["entities"])
                {
                    data.entities.push_back(enter_game_visible_entity::from_json(ent));
                }
            }
        }
        return data;
    }
};

// Convenience functions
inline json make_player_move_request(
    int32_t x, int32_t y, uint8_t direction, bool is_running = false, int32_t dest_x = -1, int32_t dest_y = -1)
{
    auto builder = message_builder(msg_type::player_move_request)
                       .set("x", x)
                       .set("y", y)
                       .set("direction", direction)
                       .set("is_running", is_running)
                       .set("timestamp",
                            static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                     std::chrono::system_clock::now().time_since_epoch())
                                                     .count()));

    // Include destination coordinates if provided (for pathfinding)
    if (dest_x >= 0 && dest_y >= 0)
    {
        builder.set("dest_x", dest_x);
        builder.set("dest_y", dest_y);
    }

    return builder.build();
}

inline json make_player_stop_request(int32_t x, int32_t y, uint8_t direction)
{
    return message_builder(msg_type::player_stop_request)
        .set("x", x)
        .set("y", y)
        .set("direction", direction)
        .set("timestamp",
             static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::system_clock::now().time_since_epoch())
                                      .count()))
        .build();
}

inline json make_entity_info_request(uint32_t entity_id)
{
    return message_builder(msg_type::entity_info_request).set("entity_id", entity_id).build();
}

inline json make_set_view_range_request(uint32_t width, uint32_t height)
{
    return message_builder(msg_type::set_view_range).set("screen_width", width).set("screen_height", height).build();
}

} // namespace hb
