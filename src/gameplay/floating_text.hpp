#pragma once

#include "graphics/text_style.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace hb {

class renderer;

// A single floating text entry (damage number, heal, status, etc.)
struct floating_text_entry
{
    std::string text;
    text_style style;
    float world_x = 0.0f;          // World-space position
    float world_y = 0.0f;
    float elapsed = 0.0f;
    float lifetime = 2.0f;
    float velocity_y = -50.0f;     // Pixels/sec (negative = upward)
    float velocity_x = 0.0f;
};

// Manages floating text that appears in world space (damage numbers, heals, etc.)
// Owned by game_state_manager, not entity_manager, because floating text
// can outlive the entity that spawned it.
class floating_text_manager
{
public:
    floating_text_manager() = default;
    ~floating_text_manager() = default;

    // Add a custom floating text entry
    void add(floating_text_entry entry);

    // Convenience: add damage number (red, floats up)
    void add_damage(int32_t amount, float world_x, float world_y);

    // Convenience: add heal number (green, floats up)
    void add_heal(int32_t amount, float world_x, float world_y);

    // Convenience: add critical hit (large red with glow, floats up faster)
    void add_critical(int32_t amount, float world_x, float world_y);

    // Update positions and remove expired entries
    void update(float delta_time);

    // Render all active floating text
    // camera_x/y: world-space camera offset for screen projection
    void render(renderer& rend, int32_t camera_x, int32_t camera_y);

    // Clear all floating text
    void clear();

private:
    std::vector<floating_text_entry> entries_;

    static constexpr size_t max_entries = 64;
};

} // namespace hb
