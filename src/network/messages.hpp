#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace hb {

using json = nlohmann::json;

// Message types
namespace msg_type {
    inline constexpr const char* login_request = "login_request";
    inline constexpr const char* login_response = "login_response";
    inline constexpr const char* get_characters_request = "get_characters_request";
    inline constexpr const char* get_characters_response = "get_characters_response";
    inline constexpr const char* enter_game_request = "enter_game_request";
    inline constexpr const char* enter_game_response = "enter_game_response";
    inline constexpr const char* create_character_request = "create_character_request";
    inline constexpr const char* create_character_response = "create_character_response";
    inline constexpr const char* delete_character_request = "delete_character_request";
    inline constexpr const char* delete_character_response = "delete_character_response";
    inline constexpr const char* set_view_range = "set_view_range";

    // Item pickup
    inline constexpr const char* player_pickup_request = "player_pickup_request";
    inline constexpr const char* player_pickup_response = "player_pickup_response";
    inline constexpr const char* ground_item_removed = "ground_item_removed";

    // Player actions
    inline constexpr const char* player_move_request = "player_move_request";
    inline constexpr const char* player_move_response = "player_move_response";
    inline constexpr const char* player_stop_request = "player_stop_request";
    inline constexpr const char* player_stop_response = "player_stop_response";
    inline constexpr const char* player_position_update = "player_position_update";

    // Player state updates
    inline constexpr const char* hunger_update = "hunger_update";
    inline constexpr const char* stat_update = "stat_update";
    inline constexpr const char* entity_hp_update = "entity_hp_update";

    // Equipment
    inline constexpr const char* player_equip_response = "player_equip_response";
    inline constexpr const char* player_unequip_response = "player_unequip_response";
    inline constexpr const char* equipment_change_broadcast = "equipment_change_broadcast";
    inline constexpr const char* inventory_data = "inventory_data";
    inline constexpr const char* equipment_data = "equipment_data";

    // Skills
    inline constexpr const char* skills_data = "skills_data";
    inline constexpr const char* player_skill_response = "player_skill_response";

    // NPC messages
    inline constexpr const char* npc_move = "npc_move";
    inline constexpr const char* npc_spawn = "npc_spawn";
    inline constexpr const char* npc_despawn = "npc_despawn";
    inline constexpr const char* npc_death = "npc_death";

    // Entity visibility
    inline constexpr const char* entity_spawn = "entity_spawn";

    // Ground items
    inline constexpr const char* ground_item_spawn = "ground_item_spawn";

    // NPC interaction
    inline constexpr const char* player_interact_response = "player_interact_response";

    // System
    inline constexpr const char* pong = "pong";
    inline constexpr const char* error_msg = "error";

    // View range (fixing literal strings in dispatch)
    inline constexpr const char* view_range_update = "view_range_update";
    inline constexpr const char* command_response = "command_response";

    // Entity info request/response (for when we receive updates for unknown entities)
    inline constexpr const char* entity_info_request = "entity_info_request";
    inline constexpr const char* entity_info_response = "entity_info_response";

    // Chat
    inline constexpr const char* chat_message = "chat_message";
    inline constexpr const char* chat_message_broadcast = "chat_message_broadcast";

    // Server-controlled render mode
    inline constexpr const char* set_render_mode = "set_render_mode";

    // Environment
    inline constexpr const char* environment_update = "environment_update";

    // Client preferences
    inline constexpr const char* set_chat_preferences = "set_chat_preferences";

    // Combat
    inline constexpr const char* player_attack_request = "player_attack_request";
    inline constexpr const char* player_attack_response = "player_attack_response";
    inline constexpr const char* combat_attack_broadcast = "combat_attack_broadcast";
    inline constexpr const char* npc_attack = "npc_attack";
    inline constexpr const char* entity_death = "entity_death";
    inline constexpr const char* entity_despawn = "entity_despawn";
    inline constexpr const char* combat_effect = "combat_effect";
    inline constexpr const char* player_death_info = "player_death_info";
    inline constexpr const char* player_respawn_request = "player_respawn_request";
    inline constexpr const char* player_teleport = "player_teleport";

    // Magic
    inline constexpr const char* player_magic_request = "player_magic_request";
    inline constexpr const char* player_magic_response = "player_magic_response";
    inline constexpr const char* spell_list_update = "spell_list_update";

    // Fishing
    inline constexpr const char* fish_skill_request = "fish_skill_request";
    inline constexpr const char* fish_skill_response = "fish_skill_response";
    inline constexpr const char* fish_engaged = "fish_engaged";
    inline constexpr const char* fish_chance_update = "fish_chance_update";
    inline constexpr const char* fish_catch_request = "fish_catch_request";
    inline constexpr const char* fish_catch_response = "fish_catch_response";
    inline constexpr const char* fish_spawn_broadcast = "fish_spawn_broadcast";
    inline constexpr const char* fish_despawn_broadcast = "fish_despawn_broadcast";
}

// Character info from server (used in get_characters_response)
struct server_character {
    int32_t id = 0;
    std::string name;
    int16_t level = 0;
    int16_t class_type = 0;     // 0=Warrior, 1=Mage, 2=Archer, etc.
    int16_t nation = 0;         // 1=Aresden, 2=Elvine

    // Appearance data
    int16_t gender = 0;         // 0=Male, 1=Female
    int16_t skin_color = 0;     // 0-3
    int16_t hair_style = 0;     // 0-7
    int16_t hair_color = 0;     // 0-15
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

    static server_character from_json(const json& j) {
        server_character c;
        if (j.contains("id")) c.id = j["id"].get<int32_t>();
        if (j.contains("name")) c.name = j["name"].get<std::string>();
        if (j.contains("level")) c.level = j["level"].get<int16_t>();
        if (j.contains("class_type")) c.class_type = j["class_type"].get<int16_t>();
        if (j.contains("nation")) c.nation = j["nation"].get<int16_t>();
        // Appearance data (optional - use defaults if not provided)
        if (j.contains("gender")) c.gender = j["gender"].get<int16_t>();
        if (j.contains("skin_color")) c.skin_color = j["skin_color"].get<int16_t>();
        if (j.contains("hair_style")) c.hair_style = j["hair_style"].get<int16_t>();
        if (j.contains("hair_color")) c.hair_color = j["hair_color"].get<int16_t>();
        if (j.contains("underwear_color")) c.underwear_color = j["underwear_color"].get<int16_t>();
        // Equipment data (optional - use defaults if not provided)
        if (j.contains("body_armor")) c.body_armor = j["body_armor"].get<uint8_t>();
        if (j.contains("arm_armor")) c.arm_armor = j["arm_armor"].get<uint8_t>();
        if (j.contains("pants")) c.pants = j["pants"].get<uint8_t>();
        if (j.contains("boots")) c.boots = j["boots"].get<uint8_t>();
        if (j.contains("helmet")) c.helmet = j["helmet"].get<uint8_t>();
        if (j.contains("mantle")) c.mantle = j["mantle"].get<uint8_t>();
        if (j.contains("weapon")) c.weapon = j["weapon"].get<uint8_t>();
        if (j.contains("shield")) c.shield = j["shield"].get<uint8_t>();
        return c;
    }
};

// Login response data
struct login_response_data {
    bool success = false;
    std::string session_token;
    std::string error_message;

    static login_response_data from_json(const json& j) {
        login_response_data data;

        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("session_token")) {
                data.session_token = d["session_token"].get<std::string>();
            }
            if (d.contains("error")) {
                data.error_message = d["error"].get<std::string>();
            }
        }

        // Error might also be at root level
        if (j.contains("error")) {
            data.error_message = j["error"].get<std::string>();
        }

        return data;
    }
};

// Get characters response data
struct get_characters_response_data {
    bool success = false;
    std::vector<server_character> characters;
    std::string error_message;

    static get_characters_response_data from_json(const json& j) {
        get_characters_response_data data;

        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("characters") && d["characters"].is_array()) {
                for (const auto& ch : d["characters"]) {
                    data.characters.push_back(server_character::from_json(ch));
                }
            }
            if (d.contains("error")) {
                data.error_message = d["error"].get<std::string>();
            }
        }

        return data;
    }
};

// === Enter game response structures (full schema) ===

// Character data from enter_game_response
struct enter_game_character {
    uint32_t id = 0;
    std::string name;
    int16_t level = 1;
    int16_t class_type = 0;      // 0=Warrior, 1=Mage
    int16_t nation = 0;          // 0=Neutral, 1=Aresden, 2=Elvine
    int16_t gender = 0;          // 0=Male, 1=Female
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
    int16_t int_ = 10;           // 'int' is reserved keyword
    int16_t mag = 10;
    int16_t cha = 10;
    int16_t hair_style = 0;
    int16_t hair_color = 0;
    int16_t skin_color = 0;
    int64_t experience = 0;
    int32_t pk_count = 0;
    int32_t hunger_level = 100;

    static enter_game_character from_json(const json& j) {
        enter_game_character c;
        if (j.contains("id")) c.id = j["id"].get<uint32_t>();
        if (j.contains("name")) c.name = j["name"].get<std::string>();
        if (j.contains("level")) c.level = j["level"].get<int16_t>();
        if (j.contains("class_type")) c.class_type = j["class_type"].get<int16_t>();
        if (j.contains("nation")) c.nation = j["nation"].get<int16_t>();
        if (j.contains("gender")) c.gender = j["gender"].get<int16_t>();
        if (j.contains("map_name")) c.map_name = j["map_name"].get<std::string>();
        if (j.contains("pos_x")) c.pos_x = j["pos_x"].get<int16_t>();
        if (j.contains("pos_y")) c.pos_y = j["pos_y"].get<int16_t>();
        if (j.contains("hp")) c.hp = j["hp"].get<int32_t>();
        if (j.contains("hp_max")) c.hp_max = j["hp_max"].get<int32_t>();
        if (j.contains("mp")) c.mp = j["mp"].get<int32_t>();
        if (j.contains("mp_max")) c.mp_max = j["mp_max"].get<int32_t>();
        if (j.contains("sp")) c.sp = j["sp"].get<int32_t>();
        if (j.contains("sp_max")) c.sp_max = j["sp_max"].get<int32_t>();
        if (j.contains("gold")) c.gold = j["gold"].get<int32_t>();
        if (j.contains("str")) c.str = j["str"].get<int16_t>();
        if (j.contains("dex")) c.dex = j["dex"].get<int16_t>();
        if (j.contains("vit")) c.vit = j["vit"].get<int16_t>();
        if (j.contains("int")) c.int_ = j["int"].get<int16_t>();
        if (j.contains("mag")) c.mag = j["mag"].get<int16_t>();
        if (j.contains("cha")) c.cha = j["cha"].get<int16_t>();
        if (j.contains("hair_style")) c.hair_style = j["hair_style"].get<int16_t>();
        if (j.contains("hair_color")) c.hair_color = j["hair_color"].get<int16_t>();
        if (j.contains("skin_color")) c.skin_color = j["skin_color"].get<int16_t>();
        if (j.contains("experience")) c.experience = j["experience"].get<int64_t>();
        if (j.contains("pk_count")) c.pk_count = j["pk_count"].get<int32_t>();
        if (j.contains("hunger_level")) c.hunger_level = j["hunger_level"].get<int32_t>();
        return c;
    }
};

// Inventory item from enter_game_response
struct enter_game_inventory_item {
    uint8_t slot = 0;
    uint32_t item_id = 0;
    std::string name;
    int16_t count = 1;
    int16_t durability = 0;
    int16_t max_durability = 0;

    static enter_game_inventory_item from_json(const json& j) {
        enter_game_inventory_item item;
        if (j.contains("slot")) item.slot = j["slot"].get<uint8_t>();
        if (j.contains("item_id")) item.item_id = j["item_id"].get<uint32_t>();
        if (j.contains("name")) item.name = j["name"].get<std::string>();
        if (j.contains("count")) item.count = j["count"].get<int16_t>();
        if (j.contains("durability")) item.durability = j["durability"].get<int16_t>();
        if (j.contains("max_durability")) item.max_durability = j["max_durability"].get<int16_t>();
        return item;
    }
};

// Inventory data from enter_game_response
struct enter_game_inventory {
    std::vector<enter_game_inventory_item> items;
    int32_t gold = 0;

    static enter_game_inventory from_json(const json& j) {
        enter_game_inventory inv;
        if (j.contains("items") && j["items"].is_array()) {
            for (const auto& item : j["items"]) {
                inv.items.push_back(enter_game_inventory_item::from_json(item));
            }
        }
        if (j.contains("gold")) inv.gold = j["gold"].get<int32_t>();
        return inv;
    }
};

// Equipment item from enter_game_response
struct enter_game_equipment_item {
    uint8_t slot = 0;            // Equipment slot (0-11)
    uint32_t item_id = 0;
    std::string name;
    int16_t durability = 0;
    int16_t max_durability = 0;

    static enter_game_equipment_item from_json(const json& j) {
        enter_game_equipment_item item;
        if (j.contains("slot")) item.slot = j["slot"].get<uint8_t>();
        if (j.contains("item_id")) item.item_id = j["item_id"].get<uint32_t>();
        if (j.contains("name")) item.name = j["name"].get<std::string>();
        if (j.contains("durability")) item.durability = j["durability"].get<int16_t>();
        if (j.contains("max_durability")) item.max_durability = j["max_durability"].get<int16_t>();
        return item;
    }
};

// Skill entry from enter_game_response
struct enter_game_skill {
    uint8_t skill_id = 0;
    int16_t level = 0;           // Skill mastery level (0-200)

    static enter_game_skill from_json(const json& j) {
        enter_game_skill skill;
        if (j.contains("skill_id")) skill.skill_id = j["skill_id"].get<uint8_t>();
        if (j.contains("level")) skill.level = j["level"].get<int16_t>();
        return skill;
    }
};

// Known spell entry from enter_game_response
struct enter_game_spell {
    uint16_t spell_id = 0;
    int16_t level = 0;           // Spell mastery level
    int32_t total_casts = 0;     // Lifetime cast count

    static enter_game_spell from_json(const json& j) {
        enter_game_spell sp;
        if (j.contains("spell_id")) sp.spell_id = j["spell_id"].get<uint16_t>();
        if (j.contains("level")) sp.level = j["level"].get<int16_t>();
        if (j.contains("total_casts")) sp.total_casts = j["total_casts"].get<int32_t>();
        return sp;
    }
};

// Quest objective from enter_game_response
struct enter_game_quest_objective {
    uint16_t id = 0;
    uint8_t status = 0;          // 0=incomplete, 1=complete, 2=failed
    int32_t current = 0;
    int32_t required = 0;

    static enter_game_quest_objective from_json(const json& j) {
        enter_game_quest_objective obj;
        if (j.contains("id")) obj.id = j["id"].get<uint16_t>();
        if (j.contains("status")) obj.status = j["status"].get<uint8_t>();
        if (j.contains("current")) obj.current = j["current"].get<int32_t>();
        if (j.contains("required")) obj.required = j["required"].get<int32_t>();
        return obj;
    }
};

// Active quest from enter_game_response
struct enter_game_active_quest {
    uint16_t quest_id = 0;
    uint8_t status = 0;          // 0=available, 1=active, 2=complete, 3=turned_in, 4=failed, 5=abandoned
    std::vector<enter_game_quest_objective> objectives;

    static enter_game_active_quest from_json(const json& j) {
        enter_game_active_quest quest;
        if (j.contains("quest_id")) quest.quest_id = j["quest_id"].get<uint16_t>();
        if (j.contains("status")) quest.status = j["status"].get<uint8_t>();
        if (j.contains("objectives") && j["objectives"].is_array()) {
            for (const auto& obj : j["objectives"]) {
                quest.objectives.push_back(enter_game_quest_objective::from_json(obj));
            }
        }
        return quest;
    }
};

// Quest data from enter_game_response
struct enter_game_quests {
    std::vector<enter_game_active_quest> active;
    std::vector<uint16_t> completed;

    static enter_game_quests from_json(const json& j) {
        enter_game_quests quests;
        if (j.contains("active") && j["active"].is_array()) {
            for (const auto& q : j["active"]) {
                quests.active.push_back(enter_game_active_quest::from_json(q));
            }
        }
        if (j.contains("completed") && j["completed"].is_array()) {
            for (const auto& id : j["completed"]) {
                quests.completed.push_back(id.get<uint16_t>());
            }
        }
        return quests;
    }
};

// Visible entity from enter_game_response world data
struct enter_game_visible_entity {
    uint32_t entity_id = 0;
    std::string type;            // "player" or "npc"
    std::string name;
    int16_t x = 0;
    int16_t y = 0;
    int16_t hp_percent = 100;
    int16_t direction = 4;       // Facing direction (0-7: N,NE,E,SE,S,SW,W,NW)
    int16_t sprite_id = 0;       // Legacy sprite type for rendering (10=Slime, etc.)
    uint32_t template_id = 0;    // NPC template ID
    int16_t level = 0;           // NPC level

    static enter_game_visible_entity from_json(const json& j) {
        enter_game_visible_entity ent;
        if (j.contains("entity_id")) ent.entity_id = j["entity_id"].get<uint32_t>();
        if (j.contains("type")) ent.type = j["type"].get<std::string>();
        if (j.contains("name")) ent.name = j["name"].get<std::string>();
        if (j.contains("x")) ent.x = j["x"].get<int16_t>();
        if (j.contains("y")) ent.y = j["y"].get<int16_t>();
        if (j.contains("hp_percent")) ent.hp_percent = j["hp_percent"].get<int16_t>();
        if (j.contains("direction")) ent.direction = j["direction"].get<int16_t>();
        if (j.contains("sprite_id")) ent.sprite_id = j["sprite_id"].get<int16_t>();
        if (j.contains("template_id")) ent.template_id = j["template_id"].get<uint32_t>();
        if (j.contains("level")) ent.level = j["level"].get<int16_t>();
        return ent;
    }
};

// World data from enter_game_response
struct enter_game_world {
    std::vector<enter_game_visible_entity> entities;

    // Environment state from server
    uint8_t time_hour = 12;
    uint8_t time_minute = 0;
    uint8_t weather = 0;

    static enter_game_world from_json(const json& j) {
        enter_game_world world;
        if (j.contains("entities") && j["entities"].is_array()) {
            for (const auto& ent : j["entities"]) {
                world.entities.push_back(enter_game_visible_entity::from_json(ent));
            }
        }
        if (j.contains("environment")) {
            const auto& env = j["environment"];
            if (env.contains("hour")) world.time_hour = env["hour"].get<uint8_t>();
            if (env.contains("minute")) world.time_minute = env["minute"].get<uint8_t>();
            if (env.contains("weather")) world.weather = env["weather"].get<uint8_t>();
        }
        return world;
    }
};

// Enter game response data (full schema)
struct enter_game_response_data {
    bool success = false;
    std::string error_message;

    // Success data
    enter_game_character character;
    enter_game_inventory inventory;
    std::vector<enter_game_equipment_item> equipment;
    std::vector<enter_game_skill> skills;
    std::vector<enter_game_spell> spells;
    enter_game_quests quests;
    enter_game_world world;

    static enter_game_response_data from_json(const json& j) {
        enter_game_response_data data;

        if (j.contains("data")) {
            const auto& d = j["data"];

            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("error")) data.error_message = d["error"].get<std::string>();

            if (data.success) {
                // Parse character data
                if (d.contains("character")) {
                    data.character = enter_game_character::from_json(d["character"]);
                }

                // Parse inventory data
                if (d.contains("inventory")) {
                    data.inventory = enter_game_inventory::from_json(d["inventory"]);
                }

                // Parse equipment array
                if (d.contains("equipment") && d["equipment"].is_array()) {
                    for (const auto& eq : d["equipment"]) {
                        data.equipment.push_back(enter_game_equipment_item::from_json(eq));
                    }
                }

                // Parse skills array
                if (d.contains("skills") && d["skills"].is_array()) {
                    for (const auto& sk : d["skills"]) {
                        data.skills.push_back(enter_game_skill::from_json(sk));
                    }
                }

                // Parse spells array
                if (d.contains("spells") && d["spells"].is_array()) {
                    for (const auto& sp : d["spells"]) {
                        data.spells.push_back(enter_game_spell::from_json(sp));
                    }
                }

                // Parse quest data
                if (d.contains("quests")) {
                    data.quests = enter_game_quests::from_json(d["quests"]);
                }

                // Parse world data
                if (d.contains("world")) {
                    data.world = enter_game_world::from_json(d["world"]);
                }
            }
        }

        return data;
    }
};

// Create character response data
struct create_character_response_data {
    bool success = false;
    int32_t character_id = 0;
    std::string error_message;

    static create_character_response_data from_json(const json& j) {
        create_character_response_data data;

        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("character_id")) data.character_id = d["character_id"].get<int32_t>();
            if (d.contains("error")) {
                data.error_message = d["error"].get<std::string>();
            }
        }

        return data;
    }
};

struct delete_character_response_data {
    bool success = false;
    std::string error_message;

    static delete_character_response_data from_json(const json& j) {
        delete_character_response_data data;

        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("error")) data.error_message = d["error"].get<std::string>();
        }

        return data;
    }
};

// Message builder helper
class message_builder {
public:
    message_builder(const char* type) {
        msg_["type"] = type;
        msg_["seq"] = next_seq_++;
    }

    template<typename T>
    message_builder& set(const std::string& key, const T& value) {
        if (!msg_.contains("data")) {
            msg_["data"] = json::object();
        }
        msg_["data"][key] = value;
        return *this;
    }

    json build() const { return msg_; }

    static void reset_sequence() { next_seq_ = 1; }

private:
    json msg_;
    static inline uint32_t next_seq_ = 1;
};

// Convenience functions
inline json make_login_request(const std::string& username, const std::string& password) {
    return message_builder(msg_type::login_request)
        .set("username", username)
        .set("password", password)
        .build();
}

inline json make_get_characters_request() {
    return message_builder(msg_type::get_characters_request)
        .build();
}

inline json make_enter_game_request(int32_t character_id, bool force_disconnect = false) {
    auto builder = message_builder(msg_type::enter_game_request)
        .set("character_id", character_id);
    if (force_disconnect) {
        builder.set("force_disconnect", true);
    }
    return builder.build();
}

inline json make_set_view_range_request(uint32_t width, uint32_t height) {
    return message_builder(msg_type::set_view_range)
        .set("screen_width", width)
        .set("screen_height", height)
        .build();
}

inline json make_set_chat_preferences_request(bool filter_profanity) {
    return message_builder(msg_type::set_chat_preferences)
        .set("filter_profanity", filter_profanity)
        .build();
}

inline json make_player_pickup_request(int32_t x, int32_t y, uint32_t item_id = 0) {
    return message_builder(msg_type::player_pickup_request)
        .set("x", x)
        .set("y", y)
        .set("item_id", item_id)
        .set("timestamp", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()))
        .build();
}

inline json make_player_move_request(int32_t x, int32_t y, uint8_t direction, bool is_running = false,
                                     int32_t dest_x = -1, int32_t dest_y = -1) {
    auto builder = message_builder(msg_type::player_move_request)
        .set("x", x)
        .set("y", y)
        .set("direction", direction)
        .set("is_running", is_running)
        .set("timestamp", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()));

    // Include destination coordinates if provided (for pathfinding)
    if (dest_x >= 0 && dest_y >= 0) {
        builder.set("dest_x", dest_x);
        builder.set("dest_y", dest_y);
    }

    return builder.build();
}

inline json make_player_stop_request(int32_t x, int32_t y, uint8_t direction) {
    return message_builder(msg_type::player_stop_request)
        .set("x", x)
        .set("y", y)
        .set("direction", direction)
        .set("timestamp", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()))
        .build();
}

inline json make_entity_info_request(uint32_t entity_id) {
    return message_builder(msg_type::entity_info_request)
        .set("entity_id", entity_id)
        .build();
}

// Pickup response data
struct player_pickup_response_data {
    bool success = false;
    uint32_t item_id = 0;
    std::string item_name;
    int16_t quantity = 0;
    uint8_t inventory_slot = 0;
    std::string error_message;

    static player_pickup_response_data from_json(const json& j) {
        player_pickup_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("error")) data.error_message = d["error"].get<std::string>();

            // Success data is nested in "result" object per protocol
            if (data.success && d.contains("result")) {
                const auto& r = d["result"];
                if (r.contains("item_id")) data.item_id = r["item_id"].get<uint32_t>();
                if (r.contains("item_name")) data.item_name = r["item_name"].get<std::string>();
                if (r.contains("quantity")) data.quantity = r["quantity"].get<int16_t>();
                if (r.contains("inventory_slot")) data.inventory_slot = r["inventory_slot"].get<uint8_t>();
            }
        }
        return data;
    }
};

// Ground item removed broadcast data
struct ground_item_removed_data {
    uint32_t picker_id = 0;
    std::string picker_name;
    uint32_t item_id = 0;
    std::string item_name;
    int32_t x = 0;
    int32_t y = 0;

    static ground_item_removed_data from_json(const json& j) {
        ground_item_removed_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("picker_id")) data.picker_id = d["picker_id"].get<uint32_t>();
            if (d.contains("picker_name")) data.picker_name = d["picker_name"].get<std::string>();
            if (d.contains("item_id")) data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("item_name")) data.item_name = d["item_name"].get<std::string>();
            if (d.contains("x")) data.x = d["x"].get<int32_t>();
            if (d.contains("y")) data.y = d["y"].get<int32_t>();
        }
        return data;
    }
};

// Player move response data (movement confirmation)
struct player_move_response_data {
    bool success = false;
    int32_t x = 0;
    int32_t y = 0;
    uint8_t direction = 0;
    std::string error;

    static player_move_response_data from_json(const json& j) {
        player_move_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("x")) data.x = d["x"].get<int32_t>();
            if (d.contains("y")) data.y = d["y"].get<int32_t>();
            if (d.contains("direction")) data.direction = d["direction"].get<uint8_t>();
            if (d.contains("error")) data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Player position update broadcast data
struct player_position_update_data {
    uint32_t entity_id = 0;
    int32_t x = 0;
    int32_t y = 0;
    uint8_t direction = 0;
    bool is_running = false;
    int32_t dest_x = -1;  // Movement destination (-1 if not provided)
    int32_t dest_y = -1;

    static player_position_update_data from_json(const json& j) {
        player_position_update_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("x")) data.x = d["x"].get<int32_t>();
            if (d.contains("y")) data.y = d["y"].get<int32_t>();
            if (d.contains("direction")) data.direction = d["direction"].get<uint8_t>();
            if (d.contains("is_running")) data.is_running = d["is_running"].get<bool>();
            if (d.contains("dest_x")) data.dest_x = d["dest_x"].get<int32_t>();
            if (d.contains("dest_y")) data.dest_y = d["dest_y"].get<int32_t>();
        }
        return data;
    }
};

// Player stop response data (direction change confirmation)
struct player_stop_response_data {
    bool success = false;
    int32_t x = 0;
    int32_t y = 0;
    uint8_t direction = 0;

    static player_stop_response_data from_json(const json& j) {
        player_stop_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("x")) data.x = d["x"].get<int32_t>();
            if (d.contains("y")) data.y = d["y"].get<int32_t>();
            if (d.contains("direction")) data.direction = d["direction"].get<uint8_t>();
        }
        return data;
    }
};

// Hunger update broadcast data
struct hunger_update_data {
    int8_t level = 100;       // Hunger level (0-100)
    bool is_starving = false; // True if level <= 0

    static hunger_update_data from_json(const json& j) {
        hunger_update_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("level")) data.level = d["level"].get<int8_t>();
            if (d.contains("is_starving")) data.is_starving = d["is_starving"].get<bool>();
        }
        return data;
    }
};

// NPC move broadcast data
struct npc_move_data {
    uint32_t entity_id = 0;
    int16_t x = 0;
    int16_t y = 0;
    uint8_t direction = 0;

    static npc_move_data from_json(const json& j) {
        npc_move_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("x")) data.x = d["x"].get<int16_t>();
            if (d.contains("y")) data.y = d["y"].get<int16_t>();
            if (d.contains("direction")) data.direction = d["direction"].get<uint8_t>();
        }
        return data;
    }
};

// Entity info response data (for both players and NPCs)
struct entity_info_response_data {
    bool success = false;
    std::string error;

    // Common fields
    uint32_t entity_id = 0;
    std::string entity_type;  // "player" or "npc"
    std::string name;
    int16_t level = 1;
    int32_t hp = 0;
    int32_t hp_max = 100;
    int16_t x = 0;
    int16_t y = 0;
    uint8_t direction = 0;

    // Player-specific fields
    std::string faction;      // "aresden", "elvine", etc.
    int16_t class_type = 0;   // 0=Warrior, 1=Mage, etc.
    int32_t pk_count = 0;
    std::string guild_name;

    // NPC-specific fields
    uint32_t template_id = 0;
    int16_t sprite_id = 0;      // Legacy sprite type for rendering (10=Slime, etc.)

    static entity_info_response_data from_json(const json& j) {
        entity_info_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("error")) data.error = d["error"].get<std::string>();

            if (data.success && d.contains("entity")) {
                const auto& e = d["entity"];
                if (e.contains("entity_id")) data.entity_id = e["entity_id"].get<uint32_t>();
                if (e.contains("entity_type")) data.entity_type = e["entity_type"].get<std::string>();
                if (e.contains("name")) data.name = e["name"].get<std::string>();
                if (e.contains("level")) data.level = e["level"].get<int16_t>();
                if (e.contains("hp")) data.hp = e["hp"].get<int32_t>();
                if (e.contains("hp_max")) data.hp_max = e["hp_max"].get<int32_t>();
                if (e.contains("x")) data.x = e["x"].get<int16_t>();
                if (e.contains("y")) data.y = e["y"].get<int16_t>();
                if (e.contains("direction")) data.direction = e["direction"].get<uint8_t>();

                // Player-specific
                if (e.contains("faction")) data.faction = e["faction"].get<std::string>();
                if (e.contains("class_type")) data.class_type = e["class_type"].get<int16_t>();
                if (e.contains("pk_count")) data.pk_count = e["pk_count"].get<int32_t>();
                if (e.contains("guild_name")) data.guild_name = e["guild_name"].get<std::string>();

                // NPC-specific
                if (e.contains("template_id")) data.template_id = e["template_id"].get<uint32_t>();
                if (e.contains("sprite_id")) data.sprite_id = e["sprite_id"].get<int16_t>();
            }
        }
        return data;
    }
};

// Chat message broadcast data (server -> client)
struct chat_message_broadcast_data {
    std::string channel;        // "local", "shout", "whisper", "guild", "party", "gm", "faction", "global"
    uint32_t sender_id = 0;
    std::string sender_name;
    std::string content;
    std::vector<std::string> flags;  // "system", "gm", "emote", etc.
    int64_t timestamp = 0;
    std::string recipient_name;      // For whispers

    static chat_message_broadcast_data from_json(const json& j) {
        chat_message_broadcast_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("channel")) data.channel = d["channel"].get<std::string>();
            if (d.contains("sender_id")) {
                if (d["sender_id"].is_number())
                    data.sender_id = d["sender_id"].get<uint32_t>();
                else if (d["sender_id"].is_string())
                    data.sender_id = static_cast<uint32_t>(std::stoul(d["sender_id"].get<std::string>()));
            }
            if (d.contains("sender_name")) data.sender_name = d["sender_name"].get<std::string>();
            if (d.contains("content")) data.content = d["content"].get<std::string>();
            if (d.contains("flags") && d["flags"].is_array()) {
                for (const auto& f : d["flags"]) {
                    data.flags.push_back(f.get<std::string>());
                }
            }
            if (d.contains("timestamp")) {
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

// Combat attack broadcast data (server -> all nearby clients)
struct combat_attack_broadcast_data {
    uint32_t attacker_id = 0;
    uint32_t target_id = 0;
    int32_t attacker_x = 0;
    int32_t attacker_y = 0;
    int32_t target_x = 0;
    int32_t target_y = 0;
    bool hit = false;
    bool critical = false;
    int32_t damage = 0;
    std::string attack_mode;      // "melee" or "ranged"
    std::string projectile_type;  // "", "arrow", or "poison_arrow"
    uint8_t direction = 0;

    bool is_ranged() const { return attack_mode == "ranged"; }

    static combat_attack_broadcast_data from_json(const json& j) {
        combat_attack_broadcast_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("attacker_id")) data.attacker_id = d["attacker_id"].get<uint32_t>();
            if (d.contains("target_id")) data.target_id = d["target_id"].get<uint32_t>();
            if (d.contains("attacker_x")) data.attacker_x = d["attacker_x"].get<int32_t>();
            if (d.contains("attacker_y")) data.attacker_y = d["attacker_y"].get<int32_t>();
            if (d.contains("target_x")) data.target_x = d["target_x"].get<int32_t>();
            if (d.contains("target_y")) data.target_y = d["target_y"].get<int32_t>();
            if (d.contains("hit")) data.hit = d["hit"].get<bool>();
            if (d.contains("critical")) data.critical = d["critical"].get<bool>();
            if (d.contains("damage")) data.damage = d["damage"].get<int32_t>();
            if (d.contains("attack_mode")) data.attack_mode = d["attack_mode"].get<std::string>();
            if (d.contains("projectile_type") && !d["projectile_type"].is_null())
                data.projectile_type = d["projectile_type"].get<std::string>();
            if (d.contains("direction")) data.direction = d["direction"].get<uint8_t>();
        }
        return data;
    }
};

// Player attack response data (server -> attacker only)
// Server nests result fields inside data.result
struct player_attack_response_data {
    bool success = false;
    std::string error;
    bool hit = false;
    bool critical = false;
    int32_t damage = 0;
    uint32_t target_id = 0;
    int32_t target_hp = 0;
    int32_t target_hp_max = 0;
    bool is_ranged = false;
    int32_t ammo_count = -1;          // remaining arrows (-1 = not applicable)
    uint32_t ammo_template_id = 0;    // which arrow type was consumed

    static player_attack_response_data from_json(const json& j) {
        player_attack_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("error")) data.error = d["error"].get<std::string>();
            // Result fields are nested inside data.result
            if (d.contains("result")) {
                const auto& r = d["result"];
                if (r.contains("hit")) data.hit = r["hit"].get<bool>();
                if (r.contains("critical")) data.critical = r["critical"].get<bool>();
                if (r.contains("damage")) data.damage = r["damage"].get<int32_t>();
                if (r.contains("target_id")) data.target_id = r["target_id"].get<uint32_t>();
                if (r.contains("target_hp")) data.target_hp = r["target_hp"].get<int32_t>();
                if (r.contains("target_hp_max")) data.target_hp_max = r["target_hp_max"].get<int32_t>();
                if (r.contains("is_ranged")) data.is_ranged = r["is_ranged"].get<bool>();
                if (r.contains("ammo_count")) data.ammo_count = r["ammo_count"].get<int32_t>();
                if (r.contains("ammo_template_id")) data.ammo_template_id = r["ammo_template_id"].get<uint32_t>();
            }
        }
        return data;
    }
};

// NPC attack data (server -> all nearby clients)
struct npc_attack_data {
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

    static npc_attack_data from_json(const json& j) {
        npc_attack_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            // Server sends "attacker_id" for npc attacks
            if (d.contains("attacker_id")) data.npc_id = d["attacker_id"].get<uint32_t>();
            if (d.contains("target_id")) data.target_id = d["target_id"].get<uint32_t>();
            if (d.contains("attacker_x")) data.npc_x = d["attacker_x"].get<int32_t>();
            if (d.contains("attacker_y")) data.npc_y = d["attacker_y"].get<int32_t>();
            if (d.contains("target_x")) data.target_x = d["target_x"].get<int32_t>();
            if (d.contains("target_y")) data.target_y = d["target_y"].get<int32_t>();
            if (d.contains("damage")) data.damage = d["damage"].get<int32_t>();
            // Server sends "is_critical" not "critical"
            if (d.contains("is_critical")) data.critical = d["is_critical"].get<bool>();
            if (d.contains("is_ranged")) data.is_ranged = d["is_ranged"].get<bool>();
            if (d.contains("projectile_type") && !d["projectile_type"].is_null())
                data.projectile_type = d["projectile_type"].get<std::string>();
        }
        return data;
    }
};

// Convenience function to build a chat message request
inline json make_chat_message_request(std::string_view content, std::string_view channel,
                                       std::string_view recipient = "")
{
    auto builder = message_builder(msg_type::chat_message)
        .set("content", std::string(content))
        .set("channel", std::string(channel));
    if (!recipient.empty()) {
        builder.set("recipient", std::string(recipient));
    }
    return builder.build();
}

// Convenience function to build a player attack request
// target_type: 1 = player, 2 = npc
inline json make_player_attack_request(uint32_t target_id, uint8_t target_type,
                                        int32_t player_x, int32_t player_y,
                                        uint8_t attack_type = 0, uint8_t direction = 0) {
    auto builder = message_builder(msg_type::player_attack_request)
        .set("target_id", target_id)
        .set("target_type", target_type)
        .set("x", player_x)
        .set("y", player_y)
        .set("attack_type", attack_type)
        .set("timestamp", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()));
    if (direction > 0)
        builder.set("direction", direction);
    return builder.build();
}

// Entity death data (server -> all nearby clients)
struct entity_death_data {
    uint32_t victim_id = 0;
    uint32_t killer_id = 0;
    int32_t x = 0;
    int32_t y = 0;

    static entity_death_data from_json(const json& j) {
        entity_death_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("victim_id")) data.victim_id = d["victim_id"].get<uint32_t>();
            if (d.contains("killer_id")) data.killer_id = d["killer_id"].get<uint32_t>();
            if (d.contains("x")) data.x = d["x"].get<int32_t>();
            if (d.contains("y")) data.y = d["y"].get<int32_t>();
        }
        return data;
    }
};

// Combat effect data (server -> all nearby clients)
struct combat_effect_data {
    uint32_t source_id = 0;
    uint32_t target_id = 0;
    std::string effect_type;     // "damage", "heal", "miss", "dodge", "block", "resist", "buff", "debuff"
    int32_t value = 0;
    std::string damage_type;     // "physical", "fire", "ice", etc.
    uint16_t spell_id = 0;
    bool is_critical = false;
    int32_t target_x = 0;
    int32_t target_y = 0;

    static combat_effect_data from_json(const json& j) {
        combat_effect_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("source_id")) data.source_id = d["source_id"].get<uint32_t>();
            if (d.contains("target_id")) data.target_id = d["target_id"].get<uint32_t>();
            if (d.contains("effect_type")) data.effect_type = d["effect_type"].get<std::string>();
            if (d.contains("value")) data.value = d["value"].get<int32_t>();
            if (d.contains("damage_type")) data.damage_type = d["damage_type"].get<std::string>();
            if (d.contains("spell_id")) data.spell_id = d["spell_id"].get<uint16_t>();
            if (d.contains("is_critical")) data.is_critical = d["is_critical"].get<bool>();
            if (d.contains("target_x")) data.target_x = d["target_x"].get<int32_t>();
            if (d.contains("target_y")) data.target_y = d["target_y"].get<int32_t>();
        }
        return data;
    }
};

// Player death info data (server -> dead player)
struct player_death_info_data {
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

    static player_death_info_data from_json(const json& j) {
        player_death_info_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("killer_id")) data.killer_id = d["killer_id"].get<uint32_t>();
            if (d.contains("killer_name")) data.killer_name = d["killer_name"].get<std::string>();
            if (d.contains("is_pvp")) data.is_pvp = d["is_pvp"].get<bool>();
            if (d.contains("xp_lost")) data.xp_lost = d["xp_lost"].get<int32_t>();
            if (d.contains("pk_points_change")) data.pk_points_change = d["pk_points_change"].get<int32_t>();
            if (d.contains("gold_reward")) data.gold_reward = d["gold_reward"].get<int32_t>();
            if (d.contains("respawn_delay_ms")) data.respawn_delay_ms = d["respawn_delay_ms"].get<int32_t>();
            if (d.contains("respawn_map")) data.respawn_map = d["respawn_map"].get<std::string>();
            if (d.contains("respawn_x")) data.respawn_x = d["respawn_x"].get<int32_t>();
            if (d.contains("respawn_y")) data.respawn_y = d["respawn_y"].get<int32_t>();
        }
        return data;
    }
};

// Player teleport data (server -> player, used for respawn and map transitions)
struct player_teleport_data {
    std::string dest_map;
    int32_t dest_x = 0;
    int32_t dest_y = 0;
    uint8_t dest_dir = 4;   // Default south
    std::vector<enter_game_visible_entity> entities;

    static player_teleport_data from_json(const json& j) {
        player_teleport_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("dest_map")) data.dest_map = d["dest_map"].get<std::string>();
            if (d.contains("dest_x")) data.dest_x = d["dest_x"].get<int32_t>();
            if (d.contains("dest_y")) data.dest_y = d["dest_y"].get<int32_t>();
            if (d.contains("dest_dir")) data.dest_dir = d["dest_dir"].get<uint8_t>();
            if (d.contains("entities") && d["entities"].is_array()) {
                for (const auto& ent : d["entities"]) {
                    data.entities.push_back(enter_game_visible_entity::from_json(ent));
                }
            }
        }
        return data;
    }
};

// Player magic response data (server -> caster)
// Server nests result fields inside data.result
struct player_magic_response_data {
    bool success = false;
    std::string error;
    uint16_t spell_id = 0;
    int32_t mana_cost = 0;
    int32_t damage = 0;
    int32_t heal = 0;
    uint32_t target_id = 0;
    int32_t caster_mp = 0;

    static player_magic_response_data from_json(const json& j) {
        player_magic_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("error")) data.error = d["error"].get<std::string>();
            if (d.contains("result")) {
                const auto& r = d["result"];
                if (r.contains("spell_id")) data.spell_id = r["spell_id"].get<uint16_t>();
                if (r.contains("mana_cost")) data.mana_cost = r["mana_cost"].get<int32_t>();
                if (r.contains("damage")) data.damage = r["damage"].get<int32_t>();
                if (r.contains("heal")) data.heal = r["heal"].get<int32_t>();
                if (r.contains("target_id")) data.target_id = r["target_id"].get<uint32_t>();
                if (r.contains("caster_mp")) data.caster_mp = r["caster_mp"].get<int32_t>();
            }
        }
        return data;
    }
};

// Convenience function to build a player magic request
inline json make_player_magic_request(int32_t x, int32_t y, uint8_t direction,
                                       uint16_t spell_id, const std::string& target_type,
                                       uint32_t target_id, int32_t target_x, int32_t target_y) {
    return message_builder(msg_type::player_magic_request)
        .set("x", x)
        .set("y", y)
        .set("direction", direction)
        .set("spell_id", spell_id)
        .set("target_type", target_type)
        .set("target_id", target_id)
        .set("target_x", target_x)
        .set("target_y", target_y)
        .set("timestamp", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()))
        .build();
}

// Convenience function to build a player respawn request
inline json make_player_respawn_request() {
    return message_builder(msg_type::player_respawn_request)
        .build();
}

// Entity spawn data (player entered visibility range)
struct entity_spawn_data {
    uint32_t entity_id = 0;
    std::string type;       // "player", "npc", "monster"
    std::string name;
    int16_t x = 0;
    int16_t y = 0;
    int32_t hp_percent = 100;
    int16_t direction = 4;  // south default

    static entity_spawn_data from_json(const json& j) {
        entity_spawn_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("type")) data.type = d["type"].get<std::string>();
            if (d.contains("name")) data.name = d["name"].get<std::string>();
            if (d.contains("x")) data.x = d["x"].get<int16_t>();
            if (d.contains("y")) data.y = d["y"].get<int16_t>();
            if (d.contains("hp_percent")) data.hp_percent = d["hp_percent"].get<int32_t>();
            if (d.contains("direction")) data.direction = d["direction"].get<int16_t>();
        }
        return data;
    }
};

// NPC spawn data (NPC/monster entered visibility range)
struct npc_spawn_data {
    uint32_t entity_id = 0;
    uint32_t template_id = 0;
    int16_t sprite_id = 0;      // Legacy sprite type for rendering (10=Slime, etc.)
    std::string name;
    int16_t x = 0;
    int16_t y = 0;
    int16_t direction = 4;
    int32_t hp = 0;
    int32_t max_hp = 0;
    int16_t level = 0;

    static npc_spawn_data from_json(const json& j) {
        npc_spawn_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("template_id")) data.template_id = d["template_id"].get<uint32_t>();
            if (d.contains("sprite_id")) data.sprite_id = d["sprite_id"].get<int16_t>();
            if (d.contains("name")) data.name = d["name"].get<std::string>();
            if (d.contains("x")) data.x = d["x"].get<int16_t>();
            if (d.contains("y")) data.y = d["y"].get<int16_t>();
            if (d.contains("direction")) data.direction = d["direction"].get<int16_t>();
            if (d.contains("hp")) data.hp = d["hp"].get<int32_t>();
            if (d.contains("max_hp")) data.max_hp = d["max_hp"].get<int32_t>();
            if (d.contains("level")) data.level = d["level"].get<int16_t>();
        }
        return data;
    }
};

// NPC despawn data
struct npc_despawn_data {
    uint32_t entity_id = 0;

    static npc_despawn_data from_json(const json& j) {
        npc_despawn_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
        }
        return data;
    }
};

// Ground item spawn data
struct ground_item_spawn_data {
    uint32_t item_id = 0;
    uint32_t template_id = 0;
    std::string item_name;
    int16_t count = 1;
    int16_t x = 0;
    int16_t y = 0;

    static ground_item_spawn_data from_json(const json& j) {
        ground_item_spawn_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("item_id")) data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("template_id")) data.template_id = d["template_id"].get<uint32_t>();
            if (d.contains("item_name")) data.item_name = d["item_name"].get<std::string>();
            if (d.contains("count")) data.count = d["count"].get<int16_t>();
            if (d.contains("x")) data.x = d["x"].get<int16_t>();
            if (d.contains("y")) data.y = d["y"].get<int16_t>();
        }
        return data;
    }
};

// Stat update data
struct stat_update_data {
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

    static stat_update_data from_json(const json& j) {
        stat_update_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("max_hp")) data.max_hp = d["max_hp"].get<int32_t>();
            if (d.contains("max_mp")) data.max_mp = d["max_mp"].get<int32_t>();
            if (d.contains("max_sp")) data.max_sp = d["max_sp"].get<int32_t>();
            if (d.contains("attack_power")) data.attack_power = d["attack_power"].get<int32_t>();
            if (d.contains("magic_power")) data.magic_power = d["magic_power"].get<int32_t>();
            if (d.contains("defense")) data.defense = d["defense"].get<int32_t>();
            if (d.contains("magic_defense")) data.magic_defense = d["magic_defense"].get<int32_t>();
            if (d.contains("hit_rate")) data.hit_rate = d["hit_rate"].get<int32_t>();
            if (d.contains("dodge_rate")) data.dodge_rate = d["dodge_rate"].get<int32_t>();
            if (d.contains("critical_rate")) data.critical_rate = d["critical_rate"].get<int32_t>();
        }
        return data;
    }
};

// Entity HP update data
struct entity_hp_update_data {
    uint32_t entity_id = 0;
    int32_t hp = 0;
    int32_t hp_max = 0;

    static entity_hp_update_data from_json(const json& j) {
        entity_hp_update_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("hp")) data.hp = d["hp"].get<int32_t>();
            if (d.contains("hp_max")) data.hp_max = d["hp_max"].get<int32_t>();
        }
        return data;
    }
};

// Equipment change broadcast data
struct equipment_change_broadcast_data {
    uint32_t entity_id = 0;
    uint8_t slot = 0;
    uint32_t item_id = 0;
    uint32_t template_id = 0;

    static equipment_change_broadcast_data from_json(const json& j) {
        equipment_change_broadcast_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("slot")) data.slot = d["slot"].get<uint8_t>();
            if (d.contains("item_id")) data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("template_id")) data.template_id = d["template_id"].get<uint32_t>();
        }
        return data;
    }
};

// Player equip response data
struct player_equip_response_data {
    bool success = false;
    uint8_t slot = 0;
    uint32_t item_id = 0;
    std::string item_name;
    int16_t durability = 0;
    int16_t max_durability = 0;
    uint32_t swapped_item_id = 0;
    uint8_t swapped_to_inv_slot = 0;
    std::string error;

    static player_equip_response_data from_json(const json& j) {
        player_equip_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("slot")) data.slot = d["slot"].get<uint8_t>();
            if (d.contains("item_id")) data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("item_name")) data.item_name = d["item_name"].get<std::string>();
            if (d.contains("durability")) data.durability = d["durability"].get<int16_t>();
            if (d.contains("max_durability")) data.max_durability = d["max_durability"].get<int16_t>();
            if (d.contains("swapped_item_id")) data.swapped_item_id = d["swapped_item_id"].get<uint32_t>();
            if (d.contains("swapped_to_inv_slot")) data.swapped_to_inv_slot = d["swapped_to_inv_slot"].get<uint8_t>();
            if (d.contains("error")) data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Player unequip response data
struct player_unequip_response_data {
    bool success = false;
    uint8_t slot = 0;
    uint32_t item_id = 0;
    std::string item_name;
    uint8_t inventory_slot = 0;
    std::string error;

    static player_unequip_response_data from_json(const json& j) {
        player_unequip_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("slot")) data.slot = d["slot"].get<uint8_t>();
            if (d.contains("item_id")) data.item_id = d["item_id"].get<uint32_t>();
            if (d.contains("item_name")) data.item_name = d["item_name"].get<std::string>();
            if (d.contains("inventory_slot")) data.inventory_slot = d["inventory_slot"].get<uint8_t>();
            if (d.contains("error")) data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// NPC death data
struct npc_death_data {
    uint32_t entity_id = 0;
    uint32_t killer_id = 0;
    int16_t x = 0;
    int16_t y = 0;

    static npc_death_data from_json(const json& j) {
        npc_death_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("entity_id")) data.entity_id = d["entity_id"].get<uint32_t>();
            if (d.contains("killer_id")) data.killer_id = d["killer_id"].get<uint32_t>();
            if (d.contains("x")) data.x = d["x"].get<int16_t>();
            if (d.contains("y")) data.y = d["y"].get<int16_t>();
        }
        return data;
    }
};

// Inventory data (full refresh)
struct inventory_data_msg {
    struct inv_item {
        uint8_t slot = 0;
        uint32_t item_id = 0;
        std::string name;
        int16_t count = 1;
        int16_t durability = 100;
        int16_t max_durability = 100;
    };
    std::vector<inv_item> items;
    int32_t gold = 0;

    static inventory_data_msg from_json(const json& j) {
        inventory_data_msg data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("gold")) data.gold = d["gold"].get<int32_t>();
            if (d.contains("items") && d["items"].is_array()) {
                for (const auto& item_j : d["items"]) {
                    inv_item itm;
                    if (item_j.contains("slot")) itm.slot = item_j["slot"].get<uint8_t>();
                    if (item_j.contains("item_id")) itm.item_id = item_j["item_id"].get<uint32_t>();
                    if (item_j.contains("name")) itm.name = item_j["name"].get<std::string>();
                    if (item_j.contains("count")) itm.count = item_j["count"].get<int16_t>();
                    if (item_j.contains("durability")) itm.durability = item_j["durability"].get<int16_t>();
                    if (item_j.contains("max_durability")) itm.max_durability = item_j["max_durability"].get<int16_t>();
                    data.items.push_back(std::move(itm));
                }
            }
        }
        return data;
    }
};

// Equipment data (full refresh)
struct equipment_data_msg {
    struct equip_item {
        uint8_t slot = 0;
        uint32_t item_id = 0;
        std::string name;
        int16_t durability = 100;
        int16_t max_durability = 100;
    };
    std::vector<equip_item> equipment;

    static equipment_data_msg from_json(const json& j) {
        equipment_data_msg data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("equipment") && d["equipment"].is_array()) {
                for (const auto& eq_j : d["equipment"]) {
                    equip_item itm;
                    if (eq_j.contains("slot")) itm.slot = eq_j["slot"].get<uint8_t>();
                    if (eq_j.contains("item_id")) itm.item_id = eq_j["item_id"].get<uint32_t>();
                    if (eq_j.contains("name")) itm.name = eq_j["name"].get<std::string>();
                    if (eq_j.contains("durability")) itm.durability = eq_j["durability"].get<int16_t>();
                    if (eq_j.contains("max_durability")) itm.max_durability = eq_j["max_durability"].get<int16_t>();
                    data.equipment.push_back(std::move(itm));
                }
            }
        }
        return data;
    }
};

// Skills data (full refresh)
struct skills_data_msg {
    struct skill_entry {
        uint8_t skill_id = 0;
        int16_t level = 0;
    };
    std::vector<skill_entry> skills;

    static skills_data_msg from_json(const json& j) {
        skills_data_msg data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("skills") && d["skills"].is_array()) {
                for (const auto& sk_j : d["skills"]) {
                    skill_entry sk;
                    if (sk_j.contains("skill_id")) sk.skill_id = sk_j["skill_id"].get<uint8_t>();
                    if (sk_j.contains("level")) sk.level = sk_j["level"].get<int16_t>();
                    data.skills.push_back(sk);
                }
            }
        }
        return data;
    }
};

// Fish skill response data
struct fish_skill_response_data {
    bool success = false;
    std::string error;

    static fish_skill_response_data from_json(const json& j) {
        fish_skill_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("success")) data.success = d["success"].get<bool>();
            if (d.contains("error")) data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Fish engaged data (fishing minigame started)
struct fish_engaged_data {
    std::string fish_name;
    uint8_t visual_type = 0;
    int32_t catch_chance = 0;

    static fish_engaged_data from_json(const json& j) {
        fish_engaged_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("fish_name")) data.fish_name = d["fish_name"].get<std::string>();
            if (d.contains("visual_type")) data.visual_type = d["visual_type"].get<uint8_t>();
            if (d.contains("catch_chance")) data.catch_chance = d["catch_chance"].get<int32_t>();
        }
        return data;
    }
};

// Fish chance update data (periodic catch chance fluctuation)
struct fish_chance_update_data {
    int32_t catch_chance = 0;

    static fish_chance_update_data from_json(const json& j) {
        fish_chance_update_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("catch_chance")) data.catch_chance = d["catch_chance"].get<int32_t>();
        }
        return data;
    }
};

// Fish catch response data
struct fish_catch_response_data {
    std::string result;          // "success", "fail", "canceled"
    std::string item_name;
    int32_t template_id = 0;
    int32_t exp_gained = 0;
    int16_t levels_gained = 0;

    static fish_catch_response_data from_json(const json& j) {
        fish_catch_response_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("result")) data.result = d["result"].get<std::string>();
            if (d.contains("item_name")) data.item_name = d["item_name"].get<std::string>();
            if (d.contains("template_id")) data.template_id = d["template_id"].get<int32_t>();
            if (d.contains("exp_gained")) data.exp_gained = d["exp_gained"].get<int32_t>();
            if (d.contains("levels_gained")) data.levels_gained = d["levels_gained"].get<int16_t>();
        }
        return data;
    }
};

// Fish spawn broadcast data (fish node appeared on map)
struct fish_spawn_broadcast_data {
    uint32_t fish_index = 0;
    uint8_t visual_type = 0;
    int16_t x = 0;
    int16_t y = 0;

    static fish_spawn_broadcast_data from_json(const json& j) {
        fish_spawn_broadcast_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("fish_index")) data.fish_index = d["fish_index"].get<uint32_t>();
            if (d.contains("visual_type")) data.visual_type = d["visual_type"].get<uint8_t>();
            if (d.contains("x")) data.x = d["x"].get<int16_t>();
            if (d.contains("y")) data.y = d["y"].get<int16_t>();
        }
        return data;
    }
};

// Fish despawn broadcast data (fish node removed from map)
struct fish_despawn_broadcast_data {
    uint32_t fish_index = 0;
    int16_t x = 0;
    int16_t y = 0;

    static fish_despawn_broadcast_data from_json(const json& j) {
        fish_despawn_broadcast_data data;
        if (j.contains("data")) {
            const auto& d = j["data"];
            if (d.contains("fish_index")) data.fish_index = d["fish_index"].get<uint32_t>();
            if (d.contains("x")) data.x = d["x"].get<int16_t>();
            if (d.contains("y")) data.y = d["y"].get<int16_t>();
        }
        return data;
    }
};

// Convenience functions for fishing requests
inline json make_fish_skill_request() {
    return message_builder(msg_type::fish_skill_request).build();
}

inline json make_fish_catch_request() {
    return message_builder(msg_type::fish_catch_request).build();
}

} // namespace hb
