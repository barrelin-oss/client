#pragma once

#include <cstdint>

namespace hb
{

class entity;
class game_state_manager;
class input;

// Handles all playing-state input: movement, combat clicks, hotkeys, pathfinding trace.
class input_handler
{
public:
    input_handler() = default;

    void initialize(game_state_manager& game);
    void handle_input(const input& inp);
    void update(float delta_time);
    void clear();

    // Movement destination accessors
    int32_t move_dest_x() const { return move_dest_x_; }
    int32_t move_dest_y() const { return move_dest_y_; }
    void set_move_dest(int32_t x, int32_t y);

    // Check if the player can perform a new action right now
    bool can_perform_action() const;

    // Blocked movement cooldown (set by server move rejection)
    void update_cooldown(float delta_time);
    void set_blocked_movement_cooldown(float v) { blocked_movement_cooldown_ = v; }
    static constexpr float blocked_movement_cooldown_duration = 0.25f;

    // Combat mode
    bool is_combat_mode() const { return combat_mode_; }
    void set_combat_mode(bool v) { combat_mode_ = v; }
    void toggle_combat_mode();
    bool is_safe_attack_mode() const { return safe_attack_mode_; }

    // Force attack mode (Ctrl+A toggle)
    bool is_force_attack_mode() const { return force_attack_mode_; }

    // Camera drag lock
    bool is_camera_drag_locked() const { return camera_drag_locked_; }
    void set_camera_drag_locked(bool v) { camera_drag_locked_ = v; }

    // Run mode
    bool is_run_mode_enabled() const { return run_mode_enabled_; }

    // Suppress input until all mouse buttons are released
    void suppress_until_release() { suppress_until_release_ = true; }

    // Spell targeting mode
    bool is_spell_targeting() const { return spell_targeting_active_; }

    // Mouse position (in view coordinates, for entity hover detection)
    int32_t mouse_x() const { return mouse_x_; }
    int32_t mouse_y() const { return mouse_y_; }
    void set_mouse_position(int32_t x, int32_t y)
    {
        mouse_x_ = x;
        mouse_y_ = y;
    }

private:
    void handle_playing_input(const input& inp);
    void handle_movement_input(const input& inp);
    void handle_combat_input(const input& inp);
    void handle_hotkey_input(const input& inp);
    void handle_spell_targeting(const input& inp);
    void update_pathfinding_trace();
    void execute_dash_attack(entity* target, const input& inp);

    game_state_manager* game_ = nullptr;

    // Movement destination for pathfinding
    int32_t move_dest_x_ = -1;
    int32_t move_dest_y_ = -1;

    // Alternates CW/CCW obstacle avoidance each step
    bool player_turn_ = false;

    // Previous tile position for doubleback detection
    int32_t prev_tile_x_ = -1;
    int32_t prev_tile_y_ = -1;

    // Mouse position for hover detection (view coordinates)
    int32_t mouse_x_ = 0;
    int32_t mouse_y_ = 0;

    // Combat mode state
    bool combat_mode_ = false;
    bool safe_attack_mode_ = false;
    bool force_attack_mode_ = false;

    // Run mode toggle
    bool run_mode_enabled_ = false;

    // Suppress input until mouse is released (prevents enter-game click from walking)
    bool suppress_until_release_ = false;

    // Camera drag lock
    bool camera_drag_locked_ = false;

    // Spell targeting mode
    bool spell_targeting_active_ = false;

    // Attack consumed this frame (prevents left-click movement when attacking)
    bool attack_consumed_ = false;

    // Blocked movement cooldown (prevents wall-spam after server rejection)
    float blocked_movement_cooldown_ = 0.0f;
};

} // namespace hb
