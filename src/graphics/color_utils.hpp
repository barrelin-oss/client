#pragma once

#include "graphics/text_style.hpp"
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

    // CPU animated effects
    rainbow = 100,    // Animated rainbow - whole text cycles together
    special = 101,    // Per-letter rainbow gradient effect
    terror = 102,     // Per-letter vertical bouncing (shaky/trembling)
    pulsing = 103,    // Alpha pulses up/down smoothly
    wave = 104,       // Smooth sinusoidal ripple per letter
    glitch = 105,     // Random letters briefly swap to other characters
    typewriter = 106, // Text reveals character by character

    // GPU shader effects
    dissolve = 110,   // Noise-based pixel dissolve with burning edge
    chromatic = 111,  // Chromatic aberration (RGB channel split)
    glow = 112,       // Bloom/glow around text
    distortion = 113, // Heat haze/distortion
    scanlines = 114,  // CRT scanline sweep
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
    return static_cast<uint8_t>(c) >= 100;
}

// Convert message_color to a text_style for the unified text renderer.
// Maps each color/effect to its visual parameters.
inline text_style style_from_message_color(message_color c)
{
    text_style style;
    style.outline_color = sf::Color::Black;
    style.outline_thickness = 1.0f;
    style.size = 18;

    switch (c)
    {
        case message_color::white:
            style.color = {225, 225, 225};
            style.effect = text_effect::none;
            break;
        case message_color::green:
            style.color = {130, 255, 130};
            style.effect = text_effect::none;
            break;
        case message_color::red:
            style.color = {255, 130, 130};
            style.effect = text_effect::none;
            break;
        case message_color::blue:
            style.color = {130, 130, 255};
            style.effect = text_effect::none;
            break;
        case message_color::yellow:
            style.color = {230, 230, 130};
            style.effect = text_effect::none;
            break;
        case message_color::system:
            style.color = {180, 255, 180};
            style.effect = text_effect::none;
            break;

        // CPU animated effects
        case message_color::rainbow:
            style.color = sf::Color::White;  // Color set by effect
            style.effect = text_effect::rainbow;
            break;
        case message_color::special:
            style.color = sf::Color::White;  // Color set by effect
            style.effect = text_effect::special;
            break;
        case message_color::terror:
            style.color = {255, 130, 130};   // Red
            style.effect = text_effect::terror;
            break;
        case message_color::pulsing:
            style.color = {230, 230, 130};   // Yellow
            style.effect = text_effect::pulsing;
            break;
        case message_color::wave:
            style.color = {130, 130, 255};   // Blue
            style.effect = text_effect::wave;
            break;
        case message_color::glitch:
            style.color = {130, 255, 130};   // Green
            style.effect = text_effect::glitch;
            break;
        case message_color::typewriter:
            style.color = {180, 255, 180};   // System green
            style.effect = text_effect::typewriter;
            break;

        // GPU shader effects
        case message_color::dissolve:
            style.color = {255, 200, 100};   // Warm orange
            style.effect = text_effect::dissolve;
            break;
        case message_color::chromatic:
            style.color = {220, 220, 255};   // Light blue-white
            style.effect = text_effect::chromatic;
            break;
        case message_color::glow:
            style.color = {100, 200, 255};   // Cyan
            style.effect = text_effect::glow;
            break;
        case message_color::distortion:
            style.color = {255, 150, 50};    // Fire orange
            style.effect = text_effect::distortion;
            break;
        case message_color::scanlines:
            style.color = {100, 255, 100};   // Terminal green
            style.effect = text_effect::scanlines;
            break;

        default:
            style.color = {225, 225, 225};
            style.effect = text_effect::none;
            break;
    }

    return style;
}

} // namespace hb
