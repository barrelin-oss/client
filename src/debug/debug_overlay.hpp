#pragma once

#include "debug/positionable.hpp"
#include "debug/position_store.hpp"
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace hb
{

class renderer;
class input;

namespace debug
{

// Main debug overlay controller for repositioning UI elements
class debug_overlay
{
public:
    static debug_overlay& instance();

    // Non-copyable
    debug_overlay(const debug_overlay&) = delete;
    debug_overlay& operator=(const debug_overlay&) = delete;

    // Initialize with position store file path
    void initialize(const std::string& position_file = "config/ui_positions.json");

    // Shutdown and save positions
    void shutdown();

    // Enable/disable overlay
    void set_enabled(bool enabled);
    void toggle();
    bool is_enabled() const { return enabled_; }

    // Register/unregister positionable elements
    void register_element(positionable* elem);
    void unregister_element(positionable* elem);
    void unregister_all();

    // Update overlay (handles input, hot-reload polling)
    // Call this every frame when overlay is enabled
    void update(float delta_time, const input& inp);

    // Render overlay (draws outlines, selection, status bar)
    // Call this every frame when overlay is enabled
    void render(renderer& rend);

    // Save positions immediately
    void save_positions();

    // Reload positions from file
    void reload_positions();

    // Undo/redo position changes
    void undo();
    void redo();
    bool can_undo() const { return !undo_stack_.empty(); }
    bool can_redo() const { return !redo_stack_.empty(); }

    // Get currently selected element (nullptr if none)
    positionable* selected_element() const;

    // Returns true if overlay consumed input this frame
    bool consumed_mouse_input() const { return consumed_mouse_input_; }
    bool consumed_keyboard_input() const { return consumed_keyboard_input_; }

private:
    debug_overlay() = default;

    void handle_keyboard_input(const input& inp);
    void handle_mouse_input(const input& inp);
    void move_selected(int32_t dx, int32_t dy);
    void select_next(bool reverse);
    void select_element_at(int32_t x, int32_t y);
    void apply_position_to_element(positionable* elem);
    void render_element_outline(renderer& rend, positionable* elem, bool selected);
    void render_status_bar(renderer& rend);

    // Record a position change for undo (call before changing position)
    void record_position_change(const std::string& id, point old_pos, point new_pos);

    std::vector<positionable*> elements_;
    int32_t selected_index_ = -1; // -1 = nothing selected

    bool enabled_ = false;
    bool initialized_ = false;

    // Hot-reload polling
    float reload_timer_ = 0.0f;
    static constexpr float reload_interval_ = 0.2f; // 200ms

    // Drag state
    bool dragging_ = false;
    int32_t drag_offset_x_ = 0;
    int32_t drag_offset_y_ = 0;

    // Status message
    std::string status_message_;
    float status_timer_ = 0.0f;

    // Input consumption tracking
    bool consumed_mouse_input_ = false;
    bool consumed_keyboard_input_ = false;

    // Undo/redo system
    struct position_change
    {
        std::string element_id;
        point old_pos;
        point new_pos;
    };
    std::deque<position_change> undo_stack_;
    std::deque<position_change> redo_stack_;
    static constexpr size_t max_undo_history_ = 100;

    // Track drag start position for undo
    point drag_start_pos_;
    bool drag_recorded_ = false;
};

} // namespace debug
} // namespace hb
