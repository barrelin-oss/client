#pragma once

#include "graphics/text_style.hpp"
#include <SFML/Graphics/Color.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <chrono>

namespace hb
{

// Chat message types
enum class chat_type : uint8_t
{
    normal = 0,   // Regular chat (white)
    shout = 1,    // Shout chat - map-wide
    whisper = 2,  // Private message
    guild = 3,    // Guild chat
    party = 4,    // Party chat
    system = 5,   // System message
    gm = 6,       // GM message
    trade = 7,    // Trade chat
    global = 8,   // Global chat
    emote = 9,    // Emote/action
    faction = 10, // Faction/town chat
};

// Get prefix for chat type
inline std::string get_chat_prefix(chat_type type)
{
    switch (type)
    {
    case chat_type::shout:
        return "[Shout] ";
    case chat_type::whisper:
        return "[Whisper] ";
    case chat_type::guild:
        return "[Guild] ";
    case chat_type::party:
        return "[Party] ";
    case chat_type::system:
        return "[System] ";
    case chat_type::gm:
        return "[GM] ";
    case chat_type::trade:
        return "[Trade] ";
    case chat_type::global:
        return "[Global] ";
    case chat_type::emote:
        return "* ";
    case chat_type::faction:
        return "[Faction] ";
    default:
        return "";
    }
}

// Get text_style for overhead chat bubble by channel name
// Colors based on legacy chat scroll colors (Game.cpp:20550-20555)
inline text_style get_chat_bubble_style(std::string_view channel)
{
    sf::Color color{230, 230, 230}; // Default: say/local (white-ish)

    if (channel == "guild")
        color = {130, 200, 130};
    else if (channel == "shout")
        color = {255, 130, 130};
    else if (channel == "party")
        color = {230, 230, 130};
    else if (channel == "faction")
        color = {130, 130, 255};
    else if (channel == "gm")
        return {{100, 255, 255}, sf::Color::Black, 1.0f, 12, text_effect::outline_pulse};
    else if (channel == "whisper")
        color = {150, 150, 170};
    else if (channel == "global")
        color = {130, 130, 255};
    else if (channel == "trade")
        color = {230, 230, 130};

    return {color, sf::Color::Black, 1.0f, 12, text_effect::none};
}

// Map chat_type enum to bubble style (single source of truth for chat colors)
inline text_style get_chat_bubble_style(chat_type type)
{
    sf::Color color{230, 230, 230};

    switch (type)
    {
    case chat_type::normal:
        color = {230, 230, 230};
        break;
    case chat_type::guild:
        color = {130, 200, 130};
        break;
    case chat_type::shout:
        color = {255, 130, 130};
        break;
    case chat_type::party:
        color = {230, 230, 130};
        break;
    case chat_type::gm:
        return {{100, 255, 255}, sf::Color::Black, 1.0f, 12, text_effect::outline_pulse};
    case chat_type::whisper:
        color = {150, 150, 170};
        break;
    case chat_type::global:
        color = {130, 130, 255};
        break;
    case chat_type::trade:
        color = {230, 230, 130};
        break;
    case chat_type::system:
        color = {255, 130, 130};
        break;
    case chat_type::emote:
        color = {200, 200, 200};
        break;
    case chat_type::faction:
        color = {130, 130, 255};
        break;
    }

    return {color, sf::Color::Black, 1.0f, 12, text_effect::none};
}

// Get color for chat type (delegates to bubble style for single source of truth)
inline sf::Color get_chat_color(chat_type type)
{
    return get_chat_bubble_style(type).color;
}

// Single chat message
struct chat_message
{
    chat_type type = chat_type::normal;
    std::string sender;
    std::string content;
    std::chrono::system_clock::time_point timestamp;

    // For whispers
    std::string recipient;

    // Formatting
    sf::Color color() const { return get_chat_color(type); }
    std::string prefix() const { return get_chat_prefix(type); }

    // Format for display
    std::string formatted() const
    {
        std::string result = prefix();

        if (!sender.empty())
        {
            if (type == chat_type::whisper && !recipient.empty())
            {
                result += "[" + sender + " -> " + recipient + "]: ";
            }
            else if (type == chat_type::emote)
            {
                result += sender + " ";
            }
            else
            {
                result += sender + ": ";
            }
        }

        result += content;
        return result;
    }
};

} // namespace hb
