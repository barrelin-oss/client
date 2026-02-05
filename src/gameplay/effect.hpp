#pragma once

#include "gameplay/effect_definition.hpp"
#include <cstdint>

namespace hb {

class sprite;

// Runtime instance of a single active effect (pool-allocated)
struct effect
{
    const effect_definition* def = nullptr;
    bool active = false;

    // Timing
    float elapsed = 0.0f;
    float frame_accumulator = 0.0f;
    int8_t current_frame = 0;
    int8_t max_frame = 0;

    // Position (world pixels)
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float prev_x = 0.0f;       // Previous position (for trails)
    float prev_y = 0.0f;

    // Source/destination (world pixels, for projectiles)
    float src_x = 0.0f;
    float src_y = 0.0f;
    float dest_x = 0.0f;
    float dest_y = 0.0f;

    // Bresenham line state (for projectile movement)
    int32_t bresenham_dx = 0;
    int32_t bresenham_dy = 0;
    int32_t bresenham_sx = 0;
    int32_t bresenham_sy = 0;
    int32_t bresenham_err = 0;

    // Physics (for particle effects)
    float velocity_x = 0.0f;
    float velocity_y = 0.0f;

    // Direction index (0-7 for directional sprites)
    uint8_t direction_index = 0;

    // Sprite reference
    const sprite* sprite_ptr = nullptr;

    // Generic value (attacker height, spawn count, etc.)
    int32_t value = 0;

    void clear()
    {
        *this = effect{};
    }
};

} // namespace hb
