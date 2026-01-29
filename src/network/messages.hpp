#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <cstdint>

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
}

// Character info from server
struct server_character {
    int32_t id = 0;
    std::string name;
    int32_t level = 0;
    std::string char_class;

    // Appearance data
    uint8_t gender = 1;         // 1 = male, 2 = female
    uint8_t skin_color = 1;     // 1-3
    uint8_t hair_style = 0;     // 0-7
    uint8_t hair_color = 0;     // 0-15
    uint8_t underwear_color = 0; // 0-7

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
        if (j.contains("level")) c.level = j["level"].get<int32_t>();
        if (j.contains("class")) c.char_class = j["class"].get<std::string>();
        // Appearance data (optional - use defaults if not provided)
        if (j.contains("gender")) c.gender = j["gender"].get<uint8_t>();
        if (j.contains("skin_color")) c.skin_color = j["skin_color"].get<uint8_t>();
        if (j.contains("hair_style")) c.hair_style = j["hair_style"].get<uint8_t>();
        if (j.contains("hair_color")) c.hair_color = j["hair_color"].get<uint8_t>();
        if (j.contains("underwear_color")) c.underwear_color = j["underwear_color"].get<uint8_t>();
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

// Visible entity from enter_game_response world data
struct enter_game_visible_entity {
    uint32_t entity_id = 0;
    std::string type;            // "player" or "npc"
    std::string name;
    int16_t x = 0;
    int16_t y = 0;
    int16_t hp_percent = 100;
    int16_t direction = 5;       // Facing direction (1-8, 0=none)

    static enter_game_visible_entity from_json(const json& j) {
        enter_game_visible_entity ent;
        if (j.contains("entity_id")) ent.entity_id = j["entity_id"].get<uint32_t>();
        if (j.contains("type")) ent.type = j["type"].get<std::string>();
        if (j.contains("name")) ent.name = j["name"].get<std::string>();
        if (j.contains("x")) ent.x = j["x"].get<int16_t>();
        if (j.contains("y")) ent.y = j["y"].get<int16_t>();
        if (j.contains("hp_percent")) ent.hp_percent = j["hp_percent"].get<int16_t>();
        if (j.contains("direction")) ent.direction = j["direction"].get<int16_t>();
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

inline json make_enter_game_request(int32_t character_id) {
    return message_builder(msg_type::enter_game_request)
        .set("character_id", character_id)
        .build();
}

} // namespace hb
