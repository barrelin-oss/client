#pragma once

#include <SFML/Graphics/Color.hpp>
#include <cstdint>
#include <cmath>

namespace hb {

// Color codes for event messages (matches legacy values + new animated effects)
enum class message_color : uint8_t
{
    white = 0,      // Default (225,225,225)
    green = 1,      // Success/positive (130,255,130)
    red = 2,        // Damage/negative (255,130,130)
    blue = 3,       // Magic/info (130,130,255)
    yellow = 4,     // Warning (230,230,130)
    system = 10,    // System messages (180,255,180)
    rainbow = 100,  // Animated rainbow - whole text cycles together
    special = 101,  // Per-letter rainbow gradient effect
    terror = 102,   // Per-letter vertical bouncing (shaky/trembling)
};

// Convert hue (0.0-1.0) to RGB color (full saturation and brightness)
inline sf::Color hue_to_rgb(float hue)
{
    hue = std::fmod(hue, 1.0f);
    if (hue < 0) hue += 1.0f;

    float r, g, b;
    int i = static_cast<int>(hue * 6.0f);
    float f = hue * 6.0f - i;
    float q = 1.0f - f;

    switch (i % 6)
    {
        case 0: r = 1.0f; g = f;    b = 0.0f; break;
        case 1: r = q;    g = 1.0f; b = 0.0f; break;
        case 2: r = 0.0f; g = 1.0f; b = f;    break;
        case 3: r = 0.0f; g = q;    b = 1.0f; break;
        case 4: r = f;    g = 0.0f; b = 1.0f; break;
        default: r = 1.0f; g = 0.0f; b = q;   break;
    }

    return sf::Color(
        static_cast<uint8_t>(r * 255),
        static_cast<uint8_t>(g * 255),
        static_cast<uint8_t>(b * 255)
    );
}

// Convert message_color to SFML color (for non-animated colors only)
inline sf::Color message_color_to_sfml(message_color c)
{
    switch (c)
    {
        case message_color::white:  return {225, 225, 225};
        case message_color::green:  return {130, 255, 130};
        case message_color::red:    return {255, 130, 130};
        case message_color::blue:   return {130, 130, 255};
        case message_color::yellow: return {230, 230, 130};
        case message_color::system: return {180, 255, 180};
        default: return {225, 225, 225};
    }
}

// Check if a message_color is animated
inline bool is_animated_color(message_color c)
{
    return c == message_color::rainbow ||
           c == message_color::special ||
           c == message_color::terror;
}

} // namespace hb
