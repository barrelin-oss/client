#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "chat/chat_message.hpp"
#include "ui/dialogs/chat_dialog.hpp"
#include "core/config.hpp"
#include "core/game_enums.hpp"
#include "graphics/renderer.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

namespace
{

time_of_day hour_to_time_of_day(uint8_t hour)
{
    if (hour < 5)
        return time_of_day::midnight;
    if (hour < 7)
        return time_of_day::dawn;
    if (hour < 10)
        return time_of_day::morning;
    if (hour < 14)
        return time_of_day::noon;
    if (hour < 17)
        return time_of_day::afternoon;
    if (hour < 19)
        return time_of_day::dusk;
    if (hour < 23)
        return time_of_day::night;
    return time_of_day::midnight;
}

} // anonymous namespace

// =============================================================================
// Chat
// =============================================================================

void ws_message_handler::send_chat_message(std::string_view content,
                                           std::string_view channel,
                                           std::string_view recipient)
{
    json msg = make_chat_message_request(content, channel, recipient);
    game_->ws_connection().send(msg);
    spdlog::debug("Sent chat: channel={} content='{}'", channel, content);
}

void ws_message_handler::handle_chat_message_broadcast(const json& message)
{
    auto data = chat_message_broadcast_data::from_json(message);

    // Skip our own messages (local echo already handled)
    auto& entities = game_->entities();
    if (data.sender_id == entities.local_player_id())
        return;

    // Map channel string to chat_type
    chat_type type = chat_type::normal;
    if (data.channel == "local")
        type = chat_type::normal;
    else if (data.channel == "shout")
        type = chat_type::shout;
    else if (data.channel == "whisper")
        type = chat_type::whisper;
    else if (data.channel == "guild")
        type = chat_type::guild;
    else if (data.channel == "party")
        type = chat_type::party;
    else if (data.channel == "gm")
        type = chat_type::gm;
    else if (data.channel == "faction")
        type = chat_type::faction;
    else if (data.channel == "global")
        type = chat_type::global;
    else if (data.channel == "trade")
        type = chat_type::trade;

    // Check flags for overrides
    for (const auto& flag : data.flags)
    {
        if (flag == "system")
        {
            type = chat_type::system;
            break;
        }
        if (flag == "gm")
        {
            type = chat_type::gm;
            break;
        }
        if (flag == "emote")
        {
            type = chat_type::emote;
            break;
        }
    }

    chat_message msg;
    msg.type = type;
    msg.sender = data.sender_name;
    msg.content = data.content;
    msg.timestamp = std::chrono::system_clock::now();
    msg.recipient = data.recipient_name;

    if (auto* chat_dlg = dynamic_cast<chat_dialog*>(game_->ui().get_dialog(dialog_type::chat)))
    {
        chat_dlg->add_message(msg);
    }

    // Set chat bubble on sender entity
    if (entity* sender = entities.get_entity(data.sender_id))
    {
        if (sender->has_name())
        {
            auto& name = sender->name();
            name.chat_message = data.content;
            name.chat_timer = 4.0f;
            name.chat_elapsed = 0.0f;
            name.chat_style = get_chat_bubble_style(data.channel);

            // Map flags to text effects
            for (const auto& flag : data.flags)
            {
                if (flag == "gm")
                {
                    name.chat_style.effect = text_effect::glow;
                    break;
                }
                if (flag == "emote")
                {
                    name.chat_style.effect = text_effect::wave;
                    break;
                }
            }
        }
    }

    spdlog::debug("Chat from '{}' [{}]: {}", data.sender_name, data.channel, data.content);
}

// =============================================================================
// Render mode / view range
// =============================================================================

void ws_message_handler::handle_set_render_mode(const json& message)
{
    if (!message.contains("data"))
    {
        spdlog::warn("set_render_mode: missing data");
        return;
    }
    const auto& d = message["data"];

    auto* rend = game_->get_renderer();
    if (!rend)
        return;

    // Parse mode
    if (d.contains("mode"))
    {
        std::string mode_str = d["mode"].get<std::string>();
        if (mode_str == "scaled")
            rend->set_view_mode(view_mode::scaled);
        else if (mode_str == "extended")
            rend->set_view_mode(view_mode::extended);
        else
            rend->set_view_mode(view_mode::special);
    }

    // Parse fair resolution
    if (d.contains("fair_width") && d.contains("fair_height"))
    {
        uint32_t fw = d["fair_width"].get<uint32_t>();
        uint32_t fh = d["fair_height"].get<uint32_t>();
        rend->set_internal_resolution(fw, fh);
    }

    // Update world screen size to match the new scene dimensions
    game_->game_world().set_screen_size(rend->scene_width(), rend->scene_height());

    // Inform server of our effective view range
    send_view_range();

    spdlog::info("Render mode set: mode={}, fair={}x{}",
                 static_cast<int>(rend->current_view_mode()),
                 rend->scene_width(),
                 rend->scene_height());
}

void ws_message_handler::handle_view_range_update(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    int16_t radius_x = d.value("radius_x", static_cast<int16_t>(40));
    int16_t radius_y = d.value("radius_y", static_cast<int16_t>(40));
    bool sees_all = d.value("sees_all", false);

    // Store visibility radii — these control how far the server streams entities,
    // NOT the fair zone size (that comes from set_render_mode).
    game_->set_view_radius(radius_x, radius_y, sees_all);

    spdlog::info("View range update: radius={}x{} tiles, sees_all={}", radius_x, radius_y, sees_all);
}

// =============================================================================
// Commands
// =============================================================================

void ws_message_handler::handle_command_response(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];
    std::string msg_text = d.value("message", "");
    bool success = d.value("success", false);

    if (!msg_text.empty())
    {
        game_->get_status_log().add_event(msg_text, success ? message_color::green : message_color::red);
    }

    spdlog::info("Command response: success={}, message={}", success, msg_text);
}

void ws_message_handler::handle_available_commands(const json& message)
{
    if (!message.contains("data") || !message["data"].contains("commands"))
    {
        spdlog::warn("available_commands: missing data.commands");
        return;
    }

    std::vector<command_entry> commands;
    for (const auto& cmd_json : message["data"]["commands"])
    {
        command_entry entry;
        if (cmd_json.contains("name"))
            entry.name = cmd_json["name"].get<std::string>();
        if (cmd_json.contains("description"))
            entry.description = cmd_json["description"].get<std::string>();
        if (cmd_json.contains("usage"))
            entry.usage = cmd_json["usage"].get<std::string>();
        if (cmd_json.contains("category"))
            entry.category = cmd_json["category"].get<std::string>();
        if (cmd_json.contains("enabled"))
            entry.enabled = cmd_json["enabled"].get<bool>();
        commands.push_back(std::move(entry));
    }

    spdlog::info("Received {} available commands from server", commands.size());
    game_->chat_input().set_commands(std::move(commands));
}

void ws_message_handler::handle_command_availability_update(const json& message)
{
    if (!message.contains("data") || !message["data"].contains("commands"))
    {
        spdlog::warn("command_availability_update: missing data.commands");
        return;
    }

    auto& chat = game_->chat_input();
    for (const auto& cmd_json : message["data"]["commands"])
    {
        if (cmd_json.contains("name") && cmd_json.contains("enabled"))
        {
            chat.update_command_availability(cmd_json["name"].get<std::string>(), cmd_json["enabled"].get<bool>());
        }
    }
}

// =============================================================================
// Environment
// =============================================================================

void ws_message_handler::handle_environment_update(const json& message)
{
    if (!game_ || !message.contains("data"))
        return;

    const auto& d = message["data"];

    // Weather (0-6 maps directly to weather_type enum)
    if (d.contains("weather"))
    {
        uint8_t w = d["weather"].get<uint8_t>();
        if (w <= static_cast<uint8_t>(weather_type::heavy_snow))
        {
            game_->game_world().set_weather(static_cast<weather_type>(w));
        }
    }

    // Time of day - map hour to time_of_day enum
    if (d.contains("hour"))
    {
        uint8_t hour = d["hour"].get<uint8_t>();
        uint8_t minute = d.contains("minute") ? d["minute"].get<uint8_t>() : 0;
        game_->game_world().set_clock(hour, minute);

        game_->game_world().set_time(hour_to_time_of_day(hour));
    }
}

// =============================================================================
// Pong / Error
// =============================================================================

void ws_message_handler::handle_pong(const json& message)
{
    if (message.contains("data"))
    {
        int64_t timestamp = message["data"].value("timestamp", static_cast<int64_t>(0));
        (void)timestamp;
        // Could calculate latency here if we stored the ping send time
    }
}

void ws_message_handler::handle_error(const json& message)
{
    if (message.contains("data"))
    {
        const auto& d = message["data"];
        std::string code = d.value("error_code", "UNKNOWN");
        std::string msg = d.value("message", "Unknown error");
        spdlog::warn("Server error [{}]: {}", code, msg);
    }
}

// =============================================================================
// Fishing
// =============================================================================

void ws_message_handler::request_fish_skill()
{
    json msg = make_fish_skill_request();
    game_->ws_connection().send(msg);
    spdlog::debug("Sent fish_skill_request");
}

void ws_message_handler::request_fish_catch()
{
    json msg = make_fish_catch_request();
    game_->ws_connection().send(msg);
    spdlog::debug("Sent fish_catch_request");
}

void ws_message_handler::handle_fish_skill_response(const json& message)
{
    auto data = fish_skill_response_data::from_json(message);

    if (!data.success)
    {
        spdlog::info("Fish skill failed: {}", data.error);
        game_->get_status_log().add_event(data.error, message_color::red);
        return;
    }

    spdlog::info("Fish skill activated - waiting for fish...");
}

void ws_message_handler::handle_fish_engaged(const json& message)
{
    auto data = fish_engaged_data::from_json(message);

    if (auto* dlg = dynamic_cast<fishing_dialog*>(game_->ui().get_dialog(dialog_type::fishing)))
    {
        dlg->open_fishing(data.fish_name, data.visual_type, data.catch_chance);
    }
}

void ws_message_handler::handle_fish_chance_update(const json& message)
{
    auto data = fish_chance_update_data::from_json(message);

    if (auto* dlg = dynamic_cast<fishing_dialog*>(game_->ui().get_dialog(dialog_type::fishing)))
    {
        if (dlg->is_open())
        {
            dlg->update_catch_chance(data.catch_chance);
        }
    }
}

void ws_message_handler::handle_fish_catch_response(const json& message)
{
    auto data = fish_catch_response_data::from_json(message);

    // Close the fishing dialog
    if (auto* dlg = dynamic_cast<fishing_dialog*>(game_->ui().get_dialog(dialog_type::fishing)))
    {
        dlg->close_fishing();
    }

    if (data.result == "success")
    {
        std::string msg = "Caught " + data.item_name + "!";
        spdlog::info("{}", msg);
        game_->get_status_log().add_event(msg, message_color::green);
    }
    else if (data.result == "fail")
    {
        spdlog::info("Fish got away!");
        game_->get_status_log().add_event("The fish got away!", message_color::yellow);
    }
    else if (data.result == "canceled")
    {
        spdlog::info("Fishing canceled");
    }
}

void ws_message_handler::handle_fish_spawn_broadcast(const json& message)
{
    auto data = fish_spawn_broadcast_data::from_json(message);
    game_->effects().add_fish_node(data.fish_index, data.x, data.y);
}

void ws_message_handler::handle_fish_despawn_broadcast(const json& message)
{
    auto data = fish_despawn_broadcast_data::from_json(message);
    game_->effects().remove_fish_node(data.fish_index);
}

} // namespace hb
