#pragma once

#include "core/game_enums.hpp"
#include "network/messages.hpp"
#include <cstdint>
#include <optional>

namespace hb {

class game_state_manager;
class entity;
class network_system;
class websocket_connection;

// Types of actions that can be queued
enum class queued_action_type : uint8_t
{
    none = 0,
    move,
    attack,
    pickup,
    magic,
    face_direction,
    stop
};

// A queued action waiting for the current animation to finish
struct queued_action
{
    queued_action_type type = queued_action_type::none;
    int32_t target_x = 0;
    int32_t target_y = 0;
    uint32_t target_id = 0;
    uint16_t spell_id = 0;
    uint8_t attack_type = 0;
    std::optional<direction> face_dir;
};

// Manages the action queue for the local player.
// Actions cannot be performed while a non-looping animation is playing,
// so they are queued and executed when the animation finishes.
class action_queue
{
public:
    action_queue() = default;

    void initialize(game_state_manager& game);
    void clear();

    // Check if the player can perform a new action right now
    bool can_perform_action() const;

    // Queue an action to execute when the current animation finishes.
    // If the player can already act, callers should execute immediately instead.
    void queue_action(queued_action action);

    // Process the pending queued action if the player can now act.
    // Called once per frame from update_playing().
    void process_pending();

    // Access the pending action (e.g. for stop override from movement input)
    queued_action& pending() { return pending_action_; }
    const queued_action& pending() const { return pending_action_; }

    // Blocked movement cooldown
    float blocked_movement_cooldown() const { return blocked_movement_cooldown_; }
    void set_blocked_movement_cooldown(float v) { blocked_movement_cooldown_ = v; }
    void update_cooldown(float delta_time);

    static constexpr float blocked_movement_cooldown_duration = 0.25f;

private:
    game_state_manager* game_ = nullptr;
    queued_action pending_action_;
    float blocked_movement_cooldown_ = 0.0f;
};

} // namespace hb
