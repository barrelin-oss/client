#pragma once

#include <SFML/Graphics/Color.hpp>
#include <cstdint>

namespace hb {

// Text effects for the unified text rendering system
// CPU effects (1-9): per-character manipulation, rendered directly
// GPU effects (10-19): render-to-texture + fragment shader
enum class text_effect : uint8_t
{
    none = 0,

    // CPU effects (per-character, render directly)
    rainbow = 1,      // Whole text cycles through hue together
    special = 2,      // Per-letter rainbow gradient
    terror = 3,       // Per-letter vertical bouncing (shaky/trembling)
    pulsing = 4,      // Alpha oscillates smoothly
    wave = 5,         // Smooth sinusoidal ripple per letter
    glitch = 6,       // Random letters briefly swap to other characters
    typewriter = 7,   // Reveals text character by character
    outline_pulse = 8,// Outline color slowly pulses brighter (text stays solid)

    // GPU effects (render-to-texture + shader)
    dissolve = 10,    // Noise-based pixel dissolve with burning edge
    chromatic = 11,   // Chromatic aberration (RGB channel split)
    glow = 12,        // Bloom/glow around text
    distortion = 13,  // Heat haze/distortion
    scanlines = 14,   // CRT scanline sweep
};

// Whether an effect requires GPU shaders
inline bool is_shader_effect(text_effect e)
{
    return static_cast<uint8_t>(e) >= 10;
}

// Whether an effect is animated (needs time parameter)
inline bool is_animated_effect(text_effect e)
{
    return e != text_effect::none;
}

// Style descriptor for text rendering
struct text_style
{
    sf::Color color = {225, 225, 225};
    sf::Color outline_color = sf::Color::Black;
    float outline_thickness = 0.0f;     // 0 = no outline
    uint32_t size = 14;
    text_effect effect = text_effect::none;
};

} // namespace hb
