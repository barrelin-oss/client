#pragma once

#include "network/messages/common.hpp"

namespace hb
{

// Fish skill response data
struct fish_skill_response_data
{
    bool success = false;
    std::string error;

    static fish_skill_response_data from_json(const json& j)
    {
        fish_skill_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("success"))
                data.success = d["success"].get<bool>();
            if (d.contains("error"))
                data.error = d["error"].get<std::string>();
        }
        return data;
    }
};

// Fish engaged data (fishing minigame started)
struct fish_engaged_data
{
    std::string fish_name;
    uint8_t visual_type = 0;
    int32_t catch_chance = 0;

    static fish_engaged_data from_json(const json& j)
    {
        fish_engaged_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("fish_name"))
                data.fish_name = d["fish_name"].get<std::string>();
            if (d.contains("visual_type"))
                data.visual_type = d["visual_type"].get<uint8_t>();
            if (d.contains("catch_chance"))
                data.catch_chance = d["catch_chance"].get<int32_t>();
        }
        return data;
    }
};

// Fish chance update data (periodic catch chance fluctuation)
struct fish_chance_update_data
{
    int32_t catch_chance = 0;

    static fish_chance_update_data from_json(const json& j)
    {
        fish_chance_update_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("catch_chance"))
                data.catch_chance = d["catch_chance"].get<int32_t>();
        }
        return data;
    }
};

// Fish catch response data
struct fish_catch_response_data
{
    std::string result; // "success", "fail", "canceled"
    std::string item_name;
    int32_t template_id = 0;

    static fish_catch_response_data from_json(const json& j)
    {
        fish_catch_response_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("result"))
                data.result = d["result"].get<std::string>();
            if (d.contains("item_name"))
                data.item_name = d["item_name"].get<std::string>();
            if (d.contains("template_id"))
                data.template_id = d["template_id"].get<int32_t>();
        }
        return data;
    }
};

// Fish spawn broadcast data (fish node appeared on map)
struct fish_spawn_broadcast_data
{
    uint32_t fish_index = 0;
    uint8_t visual_type = 0;
    int16_t x = 0;
    int16_t y = 0;

    static fish_spawn_broadcast_data from_json(const json& j)
    {
        fish_spawn_broadcast_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("fish_index"))
                data.fish_index = d["fish_index"].get<uint32_t>();
            if (d.contains("visual_type"))
                data.visual_type = d["visual_type"].get<uint8_t>();
            if (d.contains("x"))
                data.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int16_t>();
        }
        return data;
    }
};

// Fish despawn broadcast data (fish node removed from map)
struct fish_despawn_broadcast_data
{
    uint32_t fish_index = 0;
    int16_t x = 0;
    int16_t y = 0;

    static fish_despawn_broadcast_data from_json(const json& j)
    {
        fish_despawn_broadcast_data data;
        if (j.contains("data"))
        {
            const auto& d = j["data"];
            if (d.contains("fish_index"))
                data.fish_index = d["fish_index"].get<uint32_t>();
            if (d.contains("x"))
                data.x = d["x"].get<int16_t>();
            if (d.contains("y"))
                data.y = d["y"].get<int16_t>();
        }
        return data;
    }
};

// Convenience functions
inline json make_fish_skill_request()
{
    return message_builder(msg_type::fish_skill_request).build();
}

inline json make_fish_catch_request()
{
    return message_builder(msg_type::fish_catch_request).build();
}

} // namespace hb
