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

    // NPC messages
    inline constexpr const char* npc_move = "npc_move";

    // Entity info request/response (for when we receive updates for unknown entities)
    inline constexpr const char* entity_info_request = "entity_info_request";
    inline constexpr const char* entity_info_response = "entity_info_response";
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
    uint16_t visual_type = 0;    // NPC/monster type for sprite selection (10=Slime, 11=Skeleton, etc.)

    static enter_game_visible_entity from_json(const json& j) {
        enter_game_visible_entity ent;
        if (j.contains("entity_id")) ent.entity_id = j["entity_id"].get<uint32_t>();
        if (j.contains("type")) ent.type = j["type"].get<std::string>();
        if (j.contains("name")) ent.name = j["name"].get<std::string>();
        if (j.contains("x")) ent.x = j["x"].get<int16_t>();
        if (j.contains("y")) ent.y = j["y"].get<int16_t>();
        if (j.contains("hp_percent")) ent.hp_percent = j["hp_percent"].get<int16_t>();
        if (j.contains("direction")) ent.direction = j["direction"].get<int16_t>();
        if (j.contains("visual_type")) ent.visual_type = j["visual_type"].get<uint16_t>();
        return ent;
    }
};

// World data from enter_game_response
struct enter_game_world {
    std::vector<enter_game_visible_entity> entities;

    static enter_game_world from_json(const json& j) {
        enter_game_world world;
        if (j.contains("entities") && j["entities"].is_array()) {
            for (const auto& ent : j["entities"]) {
                world.entities.push_back(enter_game_visible_entity::from_json(ent));
            }
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
            }
        }
        return data;
    }
};

} // namespace hb
