#include "debug/debug_overlay.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <format>

namespace hb::debug {

debug_overlay& debug_overlay::instance()
{
    static debug_overlay overlay;
    return overlay;
}

void debug_overlay::initialize(const std::string& position_file)
{
    if (initialized_)
    {
        return;
    }

    position_store::instance().load(position_file);

    // Register for hot-reload notifications
    position_store::instance().on_position_change([this](std::string_view id, point pos) {
        // Find element with matching ID and update its position
        for (auto* elem : elements_)
        {
            if (elem->position_id() == id)
            {
                elem->set_position(pos.x, pos.y);
                spdlog::debug("Hot-reloaded position for {}: ({}, {})", id, pos.x, pos.y);
            }
        }
    });

    initialized_ = true;
    spdlog::info("Debug overlay initialized");
}

void debug_overlay::shutdown()
{
    if (!initialized_)
    {
        return;
    }

    // Save if there are unsaved changes
    if (position_store::instance().is_dirty())
    {
        position_store::instance().save();
    }

    elements_.clear();
    selected_index_ = -1;
    initialized_ = false;
}

void debug_overlay::set_enabled(bool enabled)
{
    if (enabled_ == enabled)
    {
        return;
    }

    enabled_ = enabled;
    spdlog::info("Debug overlay {}", enabled_ ? "enabled" : "disabled");

    if (enabled_)
    {
        status_message_ = "Debug overlay ON - F11:off Tab:cycle Arrows:move Ctrl+S:save";
        status_timer_ = 3.0f;
    }
}

void debug_overlay::toggle()
{
    set_enabled(!enabled_);
}

void debug_overlay::register_element(positionable* elem)
{
    if (!elem)
    {
        return;
    }

    // Avoid duplicates
    auto it = std::find(elements_.begin(), elements_.end(), elem);
    if (it != elements_.end())
    {
        return;
    }

    elements_.push_back(elem);
    spdlog::debug("Registered positionable: {}", elem->position_id());

    // Apply stored position if available
    apply_position_to_element(elem);
}

void debug_overlay::unregister_element(positionable* elem)
{
    auto it = std::find(elements_.begin(), elements_.end(), elem);
    if (it != elements_.end())
    {
        // Adjust selected index if needed
        int32_t removed_index = static_cast<int32_t>(std::distance(elements_.begin(), it));
        if (selected_index_ == removed_index)
        {
            selected_index_ = -1;
        }
        else if (selected_index_ > removed_index)
        {
            selected_index_--;
        }

        elements_.erase(it);
        spdlog::debug("Unregistered positionable: {}", elem->position_id());
    }
}

void debug_overlay::unregister_all()
{
    elements_.clear();
    selected_index_ = -1;
}

void debug_overlay::update(float delta_time, const input& inp)
{
    if (!initialized_)
    {
        return;
    }

    // F11 toggles overlay even when disabled
    if (inp.is_key_pressed(sf::Keyboard::Key::F11))
    {
        toggle();
    }

    if (!enabled_)
    {
        return;
    }

    // Hot-reload polling
    reload_timer_ += delta_time;
    if (reload_timer_ >= reload_interval_)
    {
        reload_timer_ = 0.0f;
        position_store::instance().check_for_changes();
    }

    // Update status timer
    if (status_timer_ > 0.0f)
    {
        status_timer_ -= delta_time;
    }

    // Handle input
    handle_keyboard_input(inp);
    handle_mouse_input(inp);
}

void debug_overlay::render(renderer& rend)
{
    if (!enabled_)
    {
        return;
    }

    // Draw outlines for all registered elements
    for (size_t i = 0; i < elements_.size(); ++i)
    {
        bool selected = (static_cast<int32_t>(i) == selected_index_);
        render_element_outline(rend, elements_[i], selected);
    }

    // Draw status bar
    render_status_bar(rend);
}

void debug_overlay::save_positions()
{
    if (position_store::instance().save())
    {
        status_message_ = "Positions saved";
        status_timer_ = 2.0f;
    }
    else
    {
        status_message_ = "Failed to save positions!";
        status_timer_ = 3.0f;
    }
}

void debug_overlay::reload_positions()
{
    if (position_store::instance().check_for_changes())
    {
        status_message_ = "Positions reloaded";
        status_timer_ = 2.0f;
    }
    else
    {
        // Force reload even if file hasn't changed
        std::string file_path = "config/ui_positions.json";
        if (position_store::instance().load(file_path))
        {
            // Apply all positions to elements
            for (auto* elem : elements_)
            {
                apply_position_to_element(elem);
            }
            status_message_ = "Positions reloaded";
            status_timer_ = 2.0f;
        }
        else
        {
            status_message_ = "Failed to reload positions!";
            status_timer_ = 3.0f;
        }
    }
}

positionable* debug_overlay::selected_element() const
{
    if (selected_index_ >= 0 && selected_index_ < static_cast<int32_t>(elements_.size()))
    {
        return elements_[selected_index_];
    }
    return nullptr;
}

void debug_overlay::undo()
{
    if (undo_stack_.empty())
    {
        status_message_ = "Nothing to undo";
        status_timer_ = 1.0f;
        return;
    }

    auto change = undo_stack_.back();
    undo_stack_.pop_back();

    // Find the element and restore old position
    for (auto* elem : elements_)
    {
        if (elem->position_id() == change.element_id)
        {
            elem->set_position(change.old_pos.x, change.old_pos.y);
            position_store::instance().set(change.element_id, change.old_pos);
            break;
        }
    }

    // Push to redo stack
    redo_stack_.push_back(change);
    if (redo_stack_.size() > max_undo_history_)
    {
        redo_stack_.pop_front();
    }

    status_message_ = std::format("Undo: {} -> ({}, {})",
                                   change.element_id, change.old_pos.x, change.old_pos.y);
    status_timer_ = 2.0f;
}

void debug_overlay::redo()
{
    if (redo_stack_.empty())
    {
        status_message_ = "Nothing to redo";
        status_timer_ = 1.0f;
        return;
    }

    auto change = redo_stack_.back();
    redo_stack_.pop_back();

    // Find the element and apply new position
    for (auto* elem : elements_)
    {
        if (elem->position_id() == change.element_id)
        {
            elem->set_position(change.new_pos.x, change.new_pos.y);
            position_store::instance().set(change.element_id, change.new_pos);
            break;
        }
    }

    // Push back to undo stack
    undo_stack_.push_back(change);
    if (undo_stack_.size() > max_undo_history_)
    {
        undo_stack_.pop_front();
    }

    status_message_ = std::format("Redo: {} -> ({}, {})",
                                   change.element_id, change.new_pos.x, change.new_pos.y);
    status_timer_ = 2.0f;
}

void debug_overlay::record_position_change(const std::string& id, point old_pos, point new_pos)
{
    // Don't record if position didn't actually change
    if (old_pos.x == new_pos.x && old_pos.y == new_pos.y)
    {
        return;
    }

    position_change change;
    change.element_id = id;
    change.old_pos = old_pos;
    change.new_pos = new_pos;

    undo_stack_.push_back(change);
    if (undo_stack_.size() > max_undo_history_)
    {
        undo_stack_.pop_front();
    }

    // Clear redo stack when new change is made
    redo_stack_.clear();
}

void debug_overlay::handle_keyboard_input(const input& inp)
{
    // Reset consumption flag
    consumed_keyboard_input_ = false;

    bool ctrl = inp.is_key_down(sf::Keyboard::Key::LControl) ||
                inp.is_key_down(sf::Keyboard::Key::RControl);
    bool shift = inp.is_key_down(sf::Keyboard::Key::LShift) ||
                 inp.is_key_down(sf::Keyboard::Key::RShift);

    // Ctrl+S: Save
    if (ctrl && inp.is_key_pressed(sf::Keyboard::Key::S))
    {
        save_positions();
        consumed_keyboard_input_ = true;
        return;
    }

    // Ctrl+R: Reload
    if (ctrl && inp.is_key_pressed(sf::Keyboard::Key::R))
    {
        reload_positions();
        consumed_keyboard_input_ = true;
        return;
    }

    // Ctrl+Z: Undo
    if (ctrl && !shift && inp.is_key_pressed(sf::Keyboard::Key::Z))
    {
        undo();
        consumed_keyboard_input_ = true;
        return;
    }

    // Ctrl+Shift+Z or Ctrl+Y: Redo
    if ((ctrl && shift && inp.is_key_pressed(sf::Keyboard::Key::Z)) ||
        (ctrl && inp.is_key_pressed(sf::Keyboard::Key::Y)))
    {
        redo();
        consumed_keyboard_input_ = true;
        return;
    }

    // Escape: Deselect (always consume when overlay is enabled to prevent game actions)
    if (inp.is_key_pressed(sf::Keyboard::Key::Escape))
    {
        if (selected_index_ >= 0)
        {
            selected_index_ = -1;
            status_message_ = "Deselected";
            status_timer_ = 1.0f;
        }
        consumed_keyboard_input_ = true;  // Always consume Escape when overlay is on
        return;
    }

    // Tab / Shift+Tab: Cycle through elements
    if (inp.is_key_pressed(sf::Keyboard::Key::Tab))
    {
        select_next(shift);
        consumed_keyboard_input_ = true;
        return;
    }

    // Arrow keys: Move selected element
    if (selected_index_ >= 0)
    {
        int32_t step = ctrl ? 1 : 10;

        if (inp.is_key_pressed(sf::Keyboard::Key::Up))
        {
            move_selected(0, -step);
            consumed_keyboard_input_ = true;
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Down))
        {
            move_selected(0, step);
            consumed_keyboard_input_ = true;
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Left))
        {
            move_selected(-step, 0);
            consumed_keyboard_input_ = true;
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Right))
        {
            move_selected(step, 0);
            consumed_keyboard_input_ = true;
        }
    }
}

void debug_overlay::handle_mouse_input(const input& inp)
{
    // Reset consumption flag at start of each frame's mouse handling
    consumed_mouse_input_ = false;

    int32_t mx = inp.mouse_x();
    int32_t my = inp.mouse_y();

    // Require Ctrl+Shift for overlay mouse interaction (so normal clicks pass through)
    bool modifier_held = (inp.is_key_down(sf::Keyboard::Key::LControl) ||
                          inp.is_key_down(sf::Keyboard::Key::RControl)) &&
                         (inp.is_key_down(sf::Keyboard::Key::LShift) ||
                          inp.is_key_down(sf::Keyboard::Key::RShift));

    // Ctrl+Shift+Left click: Select element under cursor
    if (modifier_held && inp.is_mouse_pressed(sf::Mouse::Button::Left))
    {
        select_element_at(mx, my);
        consumed_mouse_input_ = true;  // Consume this click

        // Start drag if we selected something
        if (selected_index_ >= 0)
        {
            auto b = elements_[selected_index_]->get_bounds();
            if (b.contains(mx, my))
            {
                dragging_ = true;
                drag_offset_x_ = mx - b.x;
                drag_offset_y_ = my - b.y;
                // Record start position for undo
                drag_start_pos_ = {b.x, b.y};
                drag_recorded_ = false;
            }
        }
    }

    // Mouse release: Stop dragging and record undo
    if (inp.is_mouse_released(sf::Mouse::Button::Left))
    {
        if (dragging_ && selected_index_ >= 0 && !drag_recorded_)
        {
            // Record the complete drag operation for undo
            auto* elem = elements_[selected_index_];
            auto b = elem->get_bounds();
            point end_pos{b.x, b.y};
            record_position_change(elem->position_id(), drag_start_pos_, end_pos);
            drag_recorded_ = true;
            consumed_mouse_input_ = true;  // Consume the release too
        }
        dragging_ = false;
    }

    // Mouse drag: Move selected element (while Ctrl+Shift held)
    if (dragging_ && modifier_held && selected_index_ >= 0 && inp.is_mouse_down(sf::Mouse::Button::Left))
    {
        consumed_mouse_input_ = true;  // Consume drag input

        int32_t new_x = mx - drag_offset_x_;
        int32_t new_y = my - drag_offset_y_;

        auto* elem = elements_[selected_index_];
        elem->set_position(new_x, new_y);

        // Update store
        position_store::instance().set(elem->position_id(), {new_x, new_y});

        // Update status
        status_message_ = std::format("{} ({}, {})", elem->position_id(), new_x, new_y);
        status_timer_ = 0.5f;
    }
    else if (dragging_ && !modifier_held)
    {
        // Stop dragging if modifiers released
        dragging_ = false;
    }
}

void debug_overlay::move_selected(int32_t dx, int32_t dy)
{
    if (selected_index_ < 0 || selected_index_ >= static_cast<int32_t>(elements_.size()))
    {
        return;
    }

    auto* elem = elements_[selected_index_];
    auto b = elem->get_bounds();
    point old_pos{b.x, b.y};
    point new_pos{b.x + dx, b.y + dy};

    elem->set_position(new_pos.x, new_pos.y);

    // Record for undo
    record_position_change(elem->position_id(), old_pos, new_pos);

    // Update store
    position_store::instance().set(elem->position_id(), new_pos);

    // Update status
    status_message_ = std::format("{} ({}, {})", elem->position_id(), new_pos.x, new_pos.y);
    status_timer_ = 1.0f;
}

void debug_overlay::select_next(bool reverse)
{
    if (elements_.empty())
    {
        return;
    }

    if (reverse)
    {
        selected_index_--;
        if (selected_index_ < 0)
        {
            selected_index_ = static_cast<int32_t>(elements_.size()) - 1;
        }
    }
    else
    {
        selected_index_++;
        if (selected_index_ >= static_cast<int32_t>(elements_.size()))
        {
            selected_index_ = 0;
        }
    }

    auto* elem = elements_[selected_index_];
    auto b = elem->get_bounds();
    status_message_ = std::format("Selected: {} ({}, {})", elem->position_id(), b.x, b.y);
    status_timer_ = 2.0f;
}

void debug_overlay::select_element_at(int32_t x, int32_t y)
{
    // Check elements in reverse order (top-most first)
    for (int32_t i = static_cast<int32_t>(elements_.size()) - 1; i >= 0; --i)
    {
        auto b = elements_[i]->get_bounds();
        if (b.contains(x, y))
        {
            selected_index_ = i;
            auto* elem = elements_[i];
            status_message_ = std::format("Selected: {} ({}, {})", elem->position_id(), b.x, b.y);
            status_timer_ = 2.0f;
            return;
        }
    }
}

void debug_overlay::apply_position_to_element(positionable* elem)
{
    auto pos = position_store::instance().get(elem->position_id());
    if (pos)
    {
        elem->set_position(pos->x, pos->y);
        spdlog::debug("Applied stored position to {}: ({}, {})",
                      elem->position_id(), pos->x, pos->y);
    }
}

void debug_overlay::render_element_outline(renderer& rend, positionable* elem, bool selected)
{
    auto b = elem->get_bounds();

    if (selected)
    {
        // Cyan border for selected
        sf::Color cyan(0, 255, 255);
        rend.draw_rect(b.x - 1, b.y - 1, b.width + 2, b.height + 2, cyan, false);
        rend.draw_rect(b.x - 2, b.y - 2, b.width + 4, b.height + 4, cyan, false);

        // Position label above element
        std::string label = std::format("{} ({}, {})", elem->position_id(), b.x, b.y);
        int32_t label_y = b.y - 18;
        if (label_y < 20)
        {
            label_y = b.y + b.height + 5;  // Below element if too close to top
        }
        rend.draw_text(label, b.x, label_y, cyan);
    }
    else
    {
        // Dim gray outline for unselected
        sf::Color gray(100, 100, 100);
        rend.draw_rect(b.x, b.y, b.width, b.height, gray, false);
    }
}

void debug_overlay::render_status_bar(renderer& rend)
{
    // Status bar at top
    sf::Color bg(0, 0, 0, 180);
    rend.draw_rect(0, 0, 640, 22, bg, true);

    sf::Color text_color(200, 200, 200);
    std::string status = "[DEBUG OVERLAY] ";

    if (status_timer_ > 0.0f && !status_message_.empty())
    {
        status += status_message_;
    }
    else
    {
        // Default hints
        if (selected_index_ >= 0 && selected_index_ < static_cast<int32_t>(elements_.size()))
        {
            auto* elem = elements_[selected_index_];
            auto b = elem->get_bounds();
            status += std::format("{} ({}, {}) | Arrows:move Ctrl+S:save Esc:deselect",
                                  elem->position_id(), b.x, b.y);
        }
        else
        {
            status += "Tab:cycle | Ctrl+Shift+Click:select | F11:close";
        }
    }

    rend.draw_text(status, 5, 4, text_color);
}

} // namespace hb::debug
