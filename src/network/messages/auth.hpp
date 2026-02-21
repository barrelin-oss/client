#pragma once

#include "gameplay/item.hpp"
#include "network/messages/common.hpp"
#include "network/messages/world.hpp"

namespace hb
{

// Character info from server (used in get_characters_response)
struct server_character
{
    int32_t id = 0;
    std::string name;
    int16_t level = 0;
    int16_t class_type = 0; // 0=Warrior, 1=Mage, 2=Archer, etc.
    int16_t nation = 0;     // 1=Aresden, 2=Elvine

    // Appearance data
    int16_t gender = 0;          // 0=Male, 1=Female
    int16_t skin_color = 0;      // 0-3
    int16_t hair_style = 0;      // 0-7
    int16_t hair_color = 0;      // 0-15
    int16_t underwear_color = 0; // 0-7

    // Equipment (0 = not equipped)
    uint8_t body_armor = 0;
    uint8_t arm_armor = 0;
    uint8_t pants = 0;
    uint8_t boots = 0;
    uint8_t helmet = 0;
    uint8_t mantle = 0;
    uint8_t weapon = 0;
    uint8_t shield = 0;

    static server_character from_json(const json& j)
    {
        server_character c;
        if (j.contains("id"))
            c.id = j["id"].get<int32_t>();
        if (j.contains("name"))
            c.name = j["name"].get<std::string>();
        if (j.contains("level"))
            c.level = j["level"].get<int16_t>();
        if (j.contains("class_type"))
            c.class_type = j["class_type"].get<int16_t>();
        if (j.contains("nation"))
            c.nation = j["nation"].get<int16_t>();
        // Appearance data (optional - use defaults if not provided)
        if (j.contains("gender"))
            c.gender = j["gender"].get<int16_t>();
        if (j.contains("skin_color"))
            c.skin_color = j["skin_color"].get<int16_t>();
        if (j.contains("hair_style"))
            c.hair_style = j["hair_style"].get<int16_t>();
        if (j.contains("hair_color"))
            c.hair_color = j["hair_color"].get<int16_t>();
        if (j.contains("underwear_color"))
            c.underwear_color = j["underwear_color"].get<int16_t>();
        // Equipment data (optional - use defaults if not provided)
        if (j.contains("body_armor"))
            c.body_armor = j["body_armor"].get<uint8_t>();
        if (j.contains("arm_armor"))
            c.arm_armor = j["arm_armor"].get<uint8_t>();
        if (j.contains("pants"))
            c.pants = j["pants"].get<uint8_t>();
        if (j.contains("boots"))
            c.boots = j["boots"].get<uint8_t>();
        if (j.contains("helmet"))
            c.helmet = j["helmet"].get<uint8_t>();
        if (j.contains("mantle"))
            c.mantle = j["mantle"].get<uint8_t>();
        if (j.contains("weapon"))
            c.weapon = j["weapon"].get<uint8_t>();
        if (j.contains("shield"))
            c.shield = j["shield"].get<uint8_t>();
        return c;
    }
};

// Login response data
struct login_response_data
{
    bool success = false;
    std::string session_token;
    std::string error_message;

    static login_response_data from_json(const json& j)
    {
        login_response_data data;

        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("session_token"))
            {
                data.session_token = d["session_token"].get<std::string>();
            }
            if (d.contains("error"))
            {
                data.error_message = d["error"].get<std::string>();
            }
        }

        // Error might also be at root level
        if (j.contains("error"))
        {
            data.error_message = j["error"].get<std::string>();
        }

        return data;
    }
};

// Get characters response data
struct get_characters_response_data
{
    bool success = false;
    std::vector<server_character> characters;
    std::string error_message;

    static get_characters_response_data from_json(const json& j)
    {
        get_characters_response_data data;

        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("characters") && d["characters"].is_array())
            {
                for (const auto& ch : d["characters"])
                {
                    data.characters.push_back(server_character::from_json(ch));
                }
            }
            if (d.contains("error"))
            {
                data.error_message = d["error"].get<std::string>();
            }
        }

        return data;
    }
};

// === Enter game response structures (full schema) ===

// Character data from enter_game_response
struct enter_game_character
{
    uint32_t id = 0;
    std::string name;
    int16_t level = 1;
    int16_t class_type = 0; // 0=Warrior, 1=Mage
    int16_t nation = 0;    // 0=neutral, 1=aresden, 2=elvine
    int16_t gender = 0;    // 0=Male, 1=Female
    std::string map_name;
    int16_t pos_x = 0;
    int16_t pos_y = 0;
    int32_t hp = 0;
    int32_t hp_max = 100;
    int32_t mp = 0;
    int32_t mp_max = 50;
    int32_t sp = 0;
    int32_t sp_max = 100;
    int32_t gold = 0;
    int16_t str = 10;
    int16_t dex = 10;
    int16_t vit = 10;
    int16_t int_ = 10; // 'int' is reserved keyword
    int16_t mag = 10;
    int16_t cha = 10;
    int16_t hair_style = 0;
    int16_t hair_color = 0;
    int16_t skin_color = 0;
    int16_t underwear_color = 0;
    int64_t experience = 0;
    int32_t pk_count = 0;
    int32_t hunger_level = 100;
    std::string guild_name;
    std::string guild_tag;
    uint8_t guild_rank = 255; // 255 = not in guild

    static enter_game_character from_json(const json& j)
    {
        enter_game_character c;
        if (j.contains("id"))
            c.id = j["id"].get<uint32_t>();
        if (j.contains("name"))
            c.name = j["name"].get<std::string>();
        if (j.contains("level"))
            c.level = j["level"].get<int16_t>();
        if (j.contains("class_type"))
            c.class_type = j["class_type"].get<int16_t>();
        if (j.contains("nation"))
            c.nation = j["nation"].get<int16_t>();
        if (j.contains("gender"))
            c.gender = j["gender"].get<int16_t>();
        if (j.contains("map_name"))
            c.map_name = j["map_name"].get<std::string>();
        if (j.contains("pos_x"))
            c.pos_x = j["pos_x"].get<int16_t>();
        if (j.contains("pos_y"))
            c.pos_y = j["pos_y"].get<int16_t>();
        if (j.contains("hp"))
            c.hp = j["hp"].get<int32_t>();
        if (j.contains("hp_max"))
            c.hp_max = j["hp_max"].get<int32_t>();
        if (j.contains("mp"))
            c.mp = j["mp"].get<int32_t>();
        if (j.contains("mp_max"))
            c.mp_max = j["mp_max"].get<int32_t>();
        if (j.contains("sp"))
            c.sp = j["sp"].get<int32_t>();
        if (j.contains("sp_max"))
            c.sp_max = j["sp_max"].get<int32_t>();
        if (j.contains("gold"))
            c.gold = j["gold"].get<int32_t>();
        if (j.contains("str"))
            c.str = j["str"].get<int16_t>();
        if (j.contains("dex"))
            c.dex = j["dex"].get<int16_t>();
        if (j.contains("vit"))
            c.vit = j["vit"].get<int16_t>();
        if (j.contains("int"))
            c.int_ = j["int"].get<int16_t>();
        if (j.contains("mag"))
            c.mag = j["mag"].get<int16_t>();
        if (j.contains("cha"))
            c.cha = j["cha"].get<int16_t>();
        if (j.contains("hair_style"))
            c.hair_style = j["hair_style"].get<int16_t>();
        if (j.contains("hair_color"))
            c.hair_color = j["hair_color"].get<int16_t>();
        if (j.contains("skin_color"))
            c.skin_color = j["skin_color"].get<int16_t>();
        if (j.contains("underwear_color"))
            c.underwear_color = j["underwear_color"].get<int16_t>();
        if (j.contains("experience"))
            c.experience = j["experience"].get<int64_t>();
        if (j.contains("pk_count"))
            c.pk_count = j["pk_count"].get<int32_t>();
        if (j.contains("hunger_level"))
            c.hunger_level = j["hunger_level"].get<int32_t>();
        if (j.contains("guild_name"))
            c.guild_name = j["guild_name"].get<std::string>();
        if (j.contains("guild_tag"))
            c.guild_tag = j["guild_tag"].get<std::string>();
        if (j.contains("guild_rank"))
            c.guild_rank = j["guild_rank"].get<uint8_t>();
        return c;
    }
};

// Inventory item from enter_game_response (matches inventory_item_msg protocol)
struct enter_game_inventory_item
{
    int16_t slot = 0;
    uint32_t item_id = 0;
    std::string name;
    uint32_t template_id = 0;
    int16_t count = 1;
    int16_t durability = 0;
    int16_t max_durability = 0;
    int16_t item_type = 0;
    int16_t equip_pos = -1;
    int16_t sprite = 0;
    int16_t sprite_frame = 0;
    int16_t color = 0;
    int16_t weight = 0;
    int16_t level_limit = 0;
    int16_t pos_x = 0;
    int16_t pos_y = 0;
    int8_t equipped_slot = -1; // -1 = not equipped
    item_attribute_data attribute;

    static enter_game_inventory_item from_json(const json& j)
    {
        enter_game_inventory_item item;
        if (j.contains("slot"))
            item.slot = j["slot"].get<int16_t>();
        if (j.contains("item_id"))
            item.item_id = j["item_id"].get<uint32_t>();
        if (j.contains("name"))
            item.name = j["name"].get<std::string>();
        if (j.contains("template_id"))
            item.template_id = j["template_id"].get<uint32_t>();
        if (j.contains("count"))
            item.count = j["count"].get<int16_t>();
        if (j.contains("durability"))
            item.durability = j["durability"].get<int16_t>();
        if (j.contains("max_durability"))
            item.max_durability = j["max_durability"].get<int16_t>();
        if (j.contains("item_type"))
            item.item_type = j["item_type"].get<int16_t>();
        if (j.contains("equip_pos"))
            item.equip_pos = j["equip_pos"].get<int16_t>();
        if (j.contains("sprite"))
            item.sprite = j["sprite"].get<int16_t>();
        if (j.contains("sprite_frame"))
            item.sprite_frame = j["sprite_frame"].get<int16_t>();
        if (j.contains("color"))
            item.color = j["color"].get<int16_t>();
        if (j.contains("weight"))
            item.weight = j["weight"].get<int16_t>();
        if (j.contains("level_limit"))
            item.level_limit = j["level_limit"].get<int16_t>();
        if (j.contains("pos_x"))
            item.pos_x = j["pos_x"].get<int16_t>();
        if (j.contains("pos_y"))
            item.pos_y = j["pos_y"].get<int16_t>();
        if (j.contains("equipped_slot"))
            item.equipped_slot = j["equipped_slot"].get<int8_t>();
        if (j.contains("attribute"))
        {
            if (j["attribute"].is_object())
                item.attribute = item_attribute_data::from_json(j["attribute"]);
            else if (j["attribute"].is_number())
                item.attribute = item_attribute_data::from_legacy(j["attribute"].get<uint32_t>());
        }
        return item;
    }
};

// Inventory data from enter_game_response
struct enter_game_inventory
{
    std::vector<enter_game_inventory_item> items;
    int32_t gold = 0;

    static enter_game_inventory from_json(const json& j)
    {
        enter_game_inventory inv;
        if (j.contains("items") && j["items"].is_array())
        {
            for (const auto& item : j["items"])
            {
                inv.items.push_back(enter_game_inventory_item::from_json(item));
            }
        }
        if (j.contains("gold"))
            inv.gold = j["gold"].get<int32_t>();
        return inv;
    }
};

// Equipment is unified with inventory — no separate enter_game_equipment_item.
// Equipped items are inventory items with equipped_slot set.

// Skill entry from enter_game_response
struct enter_game_skill
{
    uint8_t skill_id = 0;
    int16_t level = 0;              // Skill mastery level (0-200)
    int32_t total_uses = 0;         // Lifetime use count
    int32_t uses_this_level = 0;    // Uses accumulated at current level (progress numerator)
    int32_t uses_to_next_level = 0; // Uses required to reach next level (progress denominator)

    static enter_game_skill from_json(const json& j)
    {
        enter_game_skill skill;
        if (j.contains("skill_id"))
            skill.skill_id = j["skill_id"].get<uint8_t>();
        if (j.contains("level"))
            skill.level = j["level"].get<int16_t>();
        if (j.contains("total_uses"))
            skill.total_uses = j["total_uses"].get<int32_t>();
        if (j.contains("uses_this_level"))
            skill.uses_this_level = j["uses_this_level"].get<int32_t>();
        if (j.contains("uses_to_next_level"))
            skill.uses_to_next_level = j["uses_to_next_level"].get<int32_t>();
        return skill;
    }
};

// Known spell entry from enter_game_response
struct enter_game_spell
{
    uint16_t spell_id = 0;
    int16_t level = 0;       // Spell mastery level
    int32_t total_casts = 0; // Lifetime cast count

    static enter_game_spell from_json(const json& j)
    {
        enter_game_spell sp;
        if (j.contains("spell_id"))
            sp.spell_id = j["spell_id"].get<uint16_t>();
        if (j.contains("level"))
            sp.level = j["level"].get<int16_t>();
        if (j.contains("total_casts"))
            sp.total_casts = j["total_casts"].get<int32_t>();
        return sp;
    }
};

// Quest objective from enter_game_response
struct enter_game_quest_objective
{
    uint16_t id = 0;
    uint8_t status = 0; // 0=incomplete, 1=complete, 2=failed
    int32_t current = 0;
    int32_t required = 0;

    static enter_game_quest_objective from_json(const json& j)
    {
        enter_game_quest_objective obj;
        if (j.contains("id"))
            obj.id = j["id"].get<uint16_t>();
        if (j.contains("status"))
            obj.status = j["status"].get<uint8_t>();
        if (j.contains("current"))
            obj.current = j["current"].get<int32_t>();
        if (j.contains("required"))
            obj.required = j["required"].get<int32_t>();
        return obj;
    }
};

// Active quest from enter_game_response
struct enter_game_active_quest
{
    uint16_t quest_id = 0;
    uint8_t status = 0; // 0=available, 1=active, 2=complete, 3=turned_in, 4=failed, 5=abandoned
    std::vector<enter_game_quest_objective> objectives;

    static enter_game_active_quest from_json(const json& j)
    {
        enter_game_active_quest quest;
        if (j.contains("quest_id"))
            quest.quest_id = j["quest_id"].get<uint16_t>();
        if (j.contains("status"))
            quest.status = j["status"].get<uint8_t>();
        if (j.contains("objectives") && j["objectives"].is_array())
        {
            for (const auto& obj : j["objectives"])
            {
                quest.objectives.push_back(enter_game_quest_objective::from_json(obj));
            }
        }
        return quest;
    }
};

// Quest data from enter_game_response
struct enter_game_quests
{
    std::vector<enter_game_active_quest> active;
    std::vector<uint16_t> completed;

    static enter_game_quests from_json(const json& j)
    {
        enter_game_quests quests;
        if (j.contains("active") && j["active"].is_array())
        {
            for (const auto& q : j["active"])
            {
                quests.active.push_back(enter_game_active_quest::from_json(q));
            }
        }
        if (j.contains("completed") && j["completed"].is_array())
        {
            for (const auto& id : j["completed"])
            {
                quests.completed.push_back(id.get<uint16_t>());
            }
        }
        return quests;
    }
};

// World data from enter_game_response
struct enter_game_world
{
    std::vector<enter_game_visible_entity> entities;

    // Environment state from server
    uint8_t time_hour = 12;
    uint8_t time_minute = 0;
    uint8_t weather = 0;

    static enter_game_world from_json(const json& j)
    {
        enter_game_world world;
        if (j.contains("entities") && j["entities"].is_array())
        {
            for (const auto& ent : j["entities"])
            {
                world.entities.push_back(enter_game_visible_entity::from_json(ent));
            }
        }
        if (j.contains("environment"))
        {
            const auto& env = j["environment"];
            if (env.contains("hour"))
                world.time_hour = env["hour"].get<uint8_t>();
            if (env.contains("minute"))
                world.time_minute = env["minute"].get<uint8_t>();
            if (env.contains("weather"))
                world.weather = env["weather"].get<uint8_t>();
        }
        return world;
    }
};

// Enter game response data (full schema)
struct enter_game_response_data
{
    bool success = false;
    std::string error_message;

    // Success data (equipment is unified with inventory — equipped items have equipped_slot set)
    enter_game_character character;
    enter_game_inventory inventory;
    std::vector<enter_game_skill> skills;
    std::vector<enter_game_spell> spells;
    enter_game_quests quests;
    enter_game_world world;

    static enter_game_response_data from_json(const json& j)
    {
        enter_game_response_data data;

        if (j.contains("data"))
        {
            const auto& d = j["data"];

            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error_message = d["error"].get<std::string>();

            if (data.success)
            {
                // Server nests game data under "game_state"
                const auto& gs = d.contains("game_state") ? d["game_state"] : d;

                // Parse character data
                if (gs.contains("character"))
                {
                    data.character = enter_game_character::from_json(gs["character"]);
                }

                // Parse inventory data
                if (gs.contains("inventory"))
                {
                    data.inventory = enter_game_inventory::from_json(gs["inventory"]);
                }

                // Equipment is unified with inventory — no separate array.
                // Equipped items are inventory items with equipped_slot set.

                // Parse skills array
                if (gs.contains("skills") && gs["skills"].is_array())
                {
                    for (const auto& sk : gs["skills"])
                    {
                        data.skills.push_back(enter_game_skill::from_json(sk));
                    }
                }

                // Parse spells array
                if (gs.contains("spells") && gs["spells"].is_array())
                {
                    for (const auto& sp : gs["spells"])
                    {
                        data.spells.push_back(enter_game_spell::from_json(sp));
                    }
                }

                // Parse quest data
                if (gs.contains("quests"))
                {
                    data.quests = enter_game_quests::from_json(gs["quests"]);
                }

                // Parse world data
                if (gs.contains("world"))
                {
                    data.world = enter_game_world::from_json(gs["world"]);
                }
            }
        }

        return data;
    }
};

// Create character response data
struct create_character_response_data
{
    bool success = false;
    int32_t character_id = 0;
    std::string error_message;

    static create_character_response_data from_json(const json& j)
    {
        create_character_response_data data;

        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("character_id"))
                data.character_id = d["character_id"].get<int32_t>();
            if (d.contains("error"))
            {
                data.error_message = d["error"].get<std::string>();
            }
        }

        return data;
    }
};

struct delete_character_response_data
{
    bool success = false;
    std::string error_message;

    static delete_character_response_data from_json(const json& j)
    {
        delete_character_response_data data;

        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error_message = d["error"].get<std::string>();
        }

        return data;
    }
};

// Convenience functions
inline json make_login_request(const std::string& username, const std::string& password)
{
    return message_builder(msg_type::login_request).set("username", username).set("password", password).build();
}

inline json make_get_characters_request()
{
    return message_builder(msg_type::get_characters_request).build();
}

inline json make_enter_game_request(int32_t character_id, bool force_disconnect = false)
{
    auto builder = message_builder(msg_type::enter_game_request).set("character_id", character_id);
    if (force_disconnect)
    {
        builder.set("force_disconnect", true);
    }
    return builder.build();
}

} // namespace hb
