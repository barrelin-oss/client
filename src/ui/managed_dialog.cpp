#include "ui/managed_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace hb
{

managed_dialog::managed_dialog(dialog_definition def)
    : dialog(dialog_type::none) // Type not used for managed dialogs
    , definition_(std::move(def))
{
    // Set base class properties from definition
    set_title(definition_.title);
    set_bounds({definition_.bounds.x, definition_.bounds.y, definition_.bounds.width, definition_.bounds.height});
    set_modal(definition_.modal);
    set_draggable(definition_.draggable);
    set_closeable(definition_.closeable);
    set_has_border(definition_.has_border);
    set_always_on_top(definition_.always_on_top);
    set_right_click_closeable(definition_.right_click_closeable);

    // Set position ID for debug overlay (use "dialog.<yaml_id>" format)
    if (!definition_.id.empty())
    {
        set_position_id("dialog." + definition_.id);
    }

    // Apply centering if requested
    if (definition_.centered)
    {
        int32_t x = (static_cast<int32_t>(screen_width) - definition_.bounds.width) / 2;
        int32_t y = (static_cast<int32_t>(screen_height) - definition_.bounds.height) / 2;
        set_bounds({x, y, definition_.bounds.width, definition_.bounds.height});
        definition_.bounds.x = x;
        definition_.bounds.y = y;
    }

    // Initialize element states
    for (const auto& elem : definition_.elements)
    {
        element_state state;
        state.text = elem.text;
        state.visible = true;
        state.enabled = true;
        element_states_[elem.id] = std::move(state);
    }

    // Create default render strategy
    render_strategy_ = create_render_strategy(render_mode::modern);
}

// === Event callbacks ===

void managed_dialog::on_button_click(std::string_view element_id, std::function<void()> callback)
{
    button_callbacks_[std::string(element_id)] = std::move(callback);
}

void managed_dialog::on_checkbox_change(std::string_view element_id, std::function<void(bool)> callback)
{
    checkbox_callbacks_[std::string(element_id)] = std::move(callback);
}

void managed_dialog::on_text_change(std::string_view element_id, std::function<void(std::string_view)> callback)
{
    text_callbacks_[std::string(element_id)] = std::move(callback);
}

void managed_dialog::on_slider_change(std::string_view element_id, std::function<void(float)> callback)
{
    slider_callbacks_[std::string(element_id)] = std::move(callback);
}

void managed_dialog::on_grid_select(std::string_view element_id, std::function<void(int32_t, int32_t)> callback)
{
    grid_callbacks_[std::string(element_id)] = std::move(callback);
}

void managed_dialog::on_list_select(std::string_view element_id, std::function<void(int32_t)> callback)
{
    list_callbacks_[std::string(element_id)] = std::move(callback);
}

void managed_dialog::set_action_handler(action_handler handler)
{
    action_handler_ = std::move(handler);
}

// === Element access ===

void managed_dialog::set_label_text(std::string_view element_id, std::string_view text)
{
    auto& state = get_element_state(element_id);
    state.text = text;
}

void managed_dialog::set_progress(std::string_view element_id, float value)
{
    auto& state = get_element_state(element_id);
    state.progress = std::clamp(value, 0.0f, 1.0f);
}

void managed_dialog::set_checkbox_checked(std::string_view element_id, bool checked)
{
    auto& state = get_element_state(element_id);
    state.checked = checked;
}

bool managed_dialog::get_checkbox_checked(std::string_view element_id) const
{
    return get_element_state(element_id).checked;
}

void managed_dialog::set_text_input(std::string_view element_id, std::string_view text)
{
    auto& state = get_element_state(element_id);
    state.text = text;
}

std::string_view managed_dialog::get_text_input(std::string_view element_id) const
{
    return get_element_state(element_id).text;
}

void managed_dialog::set_slider_value(std::string_view element_id, float value)
{
    auto& state = get_element_state(element_id);
    state.slider_value = std::clamp(value, 0.0f, 1.0f);
}

float managed_dialog::get_slider_value(std::string_view element_id) const
{
    return get_element_state(element_id).slider_value;
}

void managed_dialog::set_element_visible(std::string_view element_id, bool visible)
{
    auto& state = get_element_state(element_id);
    state.visible = visible;
}

void managed_dialog::set_element_enabled(std::string_view element_id, bool enabled)
{
    auto& state = get_element_state(element_id);
    state.enabled = enabled;
}

bool managed_dialog::is_element_visible(std::string_view element_id) const
{
    return get_element_state(element_id).visible;
}

bool managed_dialog::is_element_enabled(std::string_view element_id) const
{
    return get_element_state(element_id).enabled;
}

void managed_dialog::set_selected_index(std::string_view element_id, int32_t index)
{
    auto& state = get_element_state(element_id);
    state.selected_index = index;
}

int32_t managed_dialog::get_selected_index(std::string_view element_id) const
{
    return get_element_state(element_id).selected_index;
}

void managed_dialog::set_button_text(std::string_view element_id, std::string_view text)
{
    auto& state = get_element_state(element_id);
    state.text = text;
}

// === Rendering mode ===

void managed_dialog::set_render_strategy(std::unique_ptr<render_strategy> strategy)
{
    render_strategy_ = std::move(strategy);
    if (render_strategy_ && sprites_)
    {
        render_strategy_->set_sprite_manager(sprites_);
    }
}

void managed_dialog::set_render_mode(render_mode mode)
{
    render_strategy_ = create_render_strategy(mode);
    if (render_strategy_ && sprites_)
    {
        render_strategy_->set_sprite_manager(sprites_);
    }
}

render_mode managed_dialog::get_render_mode() const
{
    return render_strategy_ ? render_strategy_->mode() : render_mode::modern;
}

void managed_dialog::set_sprite_manager(sprite_manager* sprites)
{
    sprites_ = sprites;
    if (render_strategy_)
    {
        render_strategy_->set_sprite_manager(sprites);
    }
}

// === dialog overrides ===

void managed_dialog::update(float delta_time, const input& inp)
{
    if (!visible_)
        return;

    // Call subclass update
    on_update_impl(delta_time);

    // Update hover states based on mouse position
    int32_t mx = inp.mouse_x();
    int32_t my = inp.mouse_y();

    // Check close button hover
    if (definition_.closeable && definition_.has_title_bar)
    {
        int32_t close_x = bounds_.x + bounds_.width - 22;
        int32_t close_y = bounds_.y + 4;
        close_button_hovered_ = (mx >= close_x && mx < close_x + 16 && my >= close_y && my < close_y + 16);
    }

    // Update element hover states
    hovered_element_id_.clear();
    const element_def* hover_elem = element_at_point(mx, my);
    if (hover_elem)
    {
        hovered_element_id_ = hover_elem->id;
    }

    // Update element states
    for (auto& [id, state] : element_states_)
    {
        state.hovered = (id == hovered_element_id_);
        state.pressed = (id == pressed_element_id_);
        state.focused = (id == focused_element_id_);
    }
}

void managed_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    // Let subclass do custom rendering first
    if (on_custom_render(rend))
    {
        return;
    }

    if (!render_strategy_)
    {
        render_strategy_ = create_render_strategy(render_mode::modern);
    }

    ui_bounds dlg_bounds = {bounds_.x, bounds_.y, bounds_.width, bounds_.height};

    // Render background
    render_strategy_->render_background(rend, definition_, dlg_bounds);

    // Render title bar
    render_strategy_->render_title_bar(rend, definition_, dlg_bounds, close_button_hovered_);

    // Render elements
    for (const auto& elem : definition_.elements)
    {
        auto it = element_states_.find(elem.id);
        if (it == element_states_.end())
            continue;

        const element_state& state = it->second;
        if (!state.visible)
            continue;

        ui_bounds abs_bounds = get_absolute_bounds(elem);
        render_strategy_->render_element(rend, elem, state, abs_bounds);
    }
}

bool managed_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    // Let subclass handle first
    if (on_custom_mouse_down(x, y, btn))
    {
        return true;
    }

    if (btn != sf::Mouse::Button::Left)
        return false;

    // Check close button
    if (definition_.closeable && definition_.has_title_bar && close_button_hovered_)
    {
        close();
        return true;
    }

    // Check title bar for dragging
    if (definition_.draggable && definition_.has_title_bar)
    {
        if (y >= bounds_.y && y < bounds_.y + title_bar_height && x >= bounds_.x && x < bounds_.x + bounds_.width)
        {
            is_dragging_ = true;
            drag_start_x_ = x - bounds_.x;
            drag_start_y_ = y - bounds_.y;
            return true;
        }
    }

    // Check elements
    const element_def* elem = element_at_point(x, y);
    if (elem)
    {
        auto& state = get_element_state(elem->id);
        if (!state.enabled)
            return false;

        pressed_element_id_ = elem->id;
        state.pressed = true;

        // Handle text input focus
        if (elem->type == element_type::text_input)
        {
            focused_element_id_ = elem->id;
        }

        // Handle checkbox toggle
        if (elem->type == element_type::checkbox)
        {
            state.checked = !state.checked;
            dispatch_checkbox_change(*elem, state.checked);
        }

        // Handle slider interaction
        if (elem->type == element_type::slider)
        {
            ui_bounds abs_bounds = get_absolute_bounds(*elem);
            float rel_x = static_cast<float>(x - abs_bounds.x) / abs_bounds.width;
            state.slider_value = std::clamp(rel_x, 0.0f, 1.0f);
            dispatch_slider_change(*elem, state.slider_value);
        }

        // Handle grid selection
        if (elem->type == element_type::grid)
        {
            auto [row, col] = grid_cell_at_point(*elem, x, y);
            if (row >= 0 && col >= 0)
            {
                state.selected_index = row * elem->grid_cols + col;
                dispatch_grid_select(*elem, row, col);
            }
        }

        return true;
    }

    // Clear focus if clicking outside any element
    focused_element_id_.clear();

    return bounds().contains(x, y);
}

bool managed_dialog::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    // Let subclass handle first
    if (on_custom_mouse_up(x, y, btn))
    {
        return true;
    }

    if (btn != sf::Mouse::Button::Left)
        return false;

    // Stop dragging
    if (is_dragging_)
    {
        is_dragging_ = false;
        return true;
    }

    // Handle button click (on release)
    if (!pressed_element_id_.empty())
    {
        const element_def* elem = find_element_def(pressed_element_id_);
        if (elem && elem->type == element_type::button)
        {
            // Only trigger if still hovering the same element
            const element_def* hover_elem = element_at_point(x, y);
            if (hover_elem && hover_elem->id == pressed_element_id_)
            {
                dispatch_button_click(*elem);
            }
        }

        // Clear pressed state
        auto& state = get_element_state(pressed_element_id_);
        state.pressed = false;
        pressed_element_id_.clear();

        return true;
    }

    return bounds().contains(x, y);
}

bool managed_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    // Let subclass handle first
    if (on_custom_mouse_move(x, y))
    {
        return true;
    }

    // Handle dragging
    if (is_dragging_)
    {
        int32_t new_x = x - drag_start_x_;
        int32_t new_y = y - drag_start_y_;

        // Clamp to screen
        new_x = std::clamp(new_x, 0, static_cast<int32_t>(screen_width) - bounds_.width);
        new_y = std::clamp(new_y, 0, static_cast<int32_t>(screen_height) - bounds_.height);

        bounds_.x = new_x;
        bounds_.y = new_y;
        definition_.bounds.x = new_x;
        definition_.bounds.y = new_y;

        return true;
    }

    // Handle slider dragging
    if (!pressed_element_id_.empty())
    {
        const element_def* elem = find_element_def(pressed_element_id_);
        if (elem && elem->type == element_type::slider)
        {
            ui_bounds abs_bounds = get_absolute_bounds(*elem);
            float rel_x = static_cast<float>(x - abs_bounds.x) / abs_bounds.width;
            auto& state = get_element_state(elem->id);
            state.slider_value = std::clamp(rel_x, 0.0f, 1.0f);
            dispatch_slider_change(*elem, state.slider_value);
            return true;
        }
    }

    return false;
}

bool managed_dialog::handle_key_press(sf::Keyboard::Key key)
{
    if (!visible_)
        return false;

    // Let subclass handle first
    if (on_custom_key_press(key))
    {
        return true;
    }

    // Handle escape to close
    if (key == sf::Keyboard::Key::Escape && definition_.closeable)
    {
        close();
        return true;
    }

    // Handle text input
    if (!focused_element_id_.empty())
    {
        const element_def* elem = find_element_def(focused_element_id_);
        if (elem && elem->type == element_type::text_input)
        {
            auto& state = get_element_state(elem->id);

            if (key == sf::Keyboard::Key::Backspace && !state.text.empty())
            {
                state.text.pop_back();
                dispatch_text_change(*elem, state.text);
                return true;
            }

            if (key == sf::Keyboard::Key::Enter)
            {
                dispatch_text_change(*elem, state.text);
                return true;
            }
        }
    }

    return false;
}

bool managed_dialog::handle_text_input(char32_t unicode)
{
    if (!visible_)
        return false;

    if (focused_element_id_.empty())
        return false;

    const element_def* elem = find_element_def(focused_element_id_);
    if (!elem || elem->type != element_type::text_input)
        return false;

    auto& state = get_element_state(elem->id);

    // Filter control characters
    if (unicode < 32 || unicode == 127)
        return false;

    // Check max length
    if (static_cast<int32_t>(state.text.length()) >= elem->max_chars)
        return false;

    // Append character
    if (unicode < 128)
    {
        state.text += static_cast<char>(unicode);
        dispatch_text_change(*elem, state.text);
        return true;
    }

    return false;
}

bool managed_dialog::handle_mouse_wheel(int32_t x, int32_t y, int32_t delta)
{
    if (!visible_)
        return false;

    // Let subclass handle it
    return on_custom_mouse_wheel(x, y, delta);
}

void managed_dialog::open()
{
    dialog::open();
    on_open_impl();
}

void managed_dialog::close()
{
    on_close_impl();
    dialog::close();
}

void managed_dialog::update_definition(const dialog_definition& new_def)
{
    // Update the definition while preserving element states and callbacks
    definition_ = new_def;

    // Update base class properties
    set_title(definition_.title);
    set_modal(definition_.modal);
    set_draggable(definition_.draggable);
    set_closeable(definition_.closeable);
    set_has_border(definition_.has_border);
    set_always_on_top(definition_.always_on_top);
    set_right_click_closeable(definition_.right_click_closeable);

    // Update bounds (apply centering if requested)
    if (definition_.centered)
    {
        int32_t x = (static_cast<int32_t>(screen_width) - definition_.bounds.width) / 2;
        int32_t y = (static_cast<int32_t>(screen_height) - definition_.bounds.height) / 2;
        set_bounds({x, y, definition_.bounds.width, definition_.bounds.height});
        definition_.bounds.x = x;
        definition_.bounds.y = y;
    }
    else
    {
        set_bounds({definition_.bounds.x, definition_.bounds.y, definition_.bounds.width, definition_.bounds.height});
    }

    // Add states for any new elements (preserve existing states)
    for (const auto& elem : definition_.elements)
    {
        if (element_states_.find(elem.id) == element_states_.end())
        {
            element_state state;
            state.text = elem.text;
            state.visible = true;
            state.enabled = true;
            element_states_[elem.id] = std::move(state);
        }
    }

    spdlog::debug("Updated definition for dialog '{}'", definition_.id);
}

// === Protected helpers ===

element_state& managed_dialog::get_element_state(std::string_view id)
{
    auto it = element_states_.find(std::string(id));
    if (it != element_states_.end())
    {
        return it->second;
    }

    // Create default state
    static element_state default_state;
    element_states_[std::string(id)] = element_state{};
    return element_states_[std::string(id)];
}

const element_state& managed_dialog::get_element_state(std::string_view id) const
{
    auto it = element_states_.find(std::string(id));
    if (it != element_states_.end())
    {
        return it->second;
    }

    static const element_state default_state;
    return default_state;
}

// === Private helpers ===

const element_def* managed_dialog::find_element_def(std::string_view id) const
{
    return definition_.find_element(id);
}

ui_bounds managed_dialog::get_absolute_bounds(const element_def& elem) const
{
    int32_t title_offset = definition_.has_title_bar ? title_bar_height : 0;
    return {bounds_.x + elem.bounds.x, bounds_.y + title_offset + elem.bounds.y, elem.bounds.width, elem.bounds.height};
}

const element_def* managed_dialog::element_at_point(int32_t x, int32_t y) const
{
    // Check in reverse order (front to back)
    for (auto it = definition_.elements.rbegin(); it != definition_.elements.rend(); ++it)
    {
        const auto& elem = *it;

        auto state_it = element_states_.find(elem.id);
        if (state_it == element_states_.end() || !state_it->second.visible)
        {
            continue;
        }

        ui_bounds abs_bounds = get_absolute_bounds(elem);
        if (abs_bounds.contains(x, y))
        {
            return &elem;
        }
    }
    return nullptr;
}

std::pair<int32_t, int32_t> managed_dialog::grid_cell_at_point(const element_def& elem, int32_t x, int32_t y) const
{
    ui_bounds abs_bounds = get_absolute_bounds(elem);

    int32_t rel_x = x - abs_bounds.x;
    int32_t rel_y = y - abs_bounds.y;

    if (rel_x < 0 || rel_y < 0)
        return {-1, -1};

    int32_t cell_step_x = elem.cell_width + elem.cell_padding;
    int32_t cell_step_y = elem.cell_height + elem.cell_padding;

    int32_t col = rel_x / cell_step_x;
    int32_t row = rel_y / cell_step_y;

    // Check if within a cell (not in padding)
    int32_t cell_x = rel_x % cell_step_x;
    int32_t cell_y = rel_y % cell_step_y;

    if (cell_x >= elem.cell_width || cell_y >= elem.cell_height)
    {
        return {-1, -1}; // In padding
    }

    if (col >= 0 && col < elem.grid_cols && row >= 0 && row < elem.grid_rows)
    {
        return {row, col};
    }

    return {-1, -1};
}

void managed_dialog::dispatch_button_click(const element_def& elem)
{
    // Execute code callback first
    auto it = button_callbacks_.find(elem.id);
    if (it != button_callbacks_.end() && it->second)
    {
        it->second();
    }

    // Then execute YAML-based action
    if (elem.action != element_action::none && action_handler_)
    {
        if (elem.action == element_action::close_self)
        {
            // close_self uses this dialog's ID
            action_handler_(elem.action, definition_.id);
        }
        else
        {
            action_handler_(elem.action, elem.action_target);
        }
    }
}

void managed_dialog::dispatch_checkbox_change(const element_def& elem, bool checked)
{
    auto it = checkbox_callbacks_.find(elem.id);
    if (it != checkbox_callbacks_.end() && it->second)
    {
        it->second(checked);
    }
}

void managed_dialog::dispatch_text_change(const element_def& elem, std::string_view text)
{
    auto it = text_callbacks_.find(elem.id);
    if (it != text_callbacks_.end() && it->second)
    {
        it->second(text);
    }
}

void managed_dialog::dispatch_slider_change(const element_def& elem, float value)
{
    auto it = slider_callbacks_.find(elem.id);
    if (it != slider_callbacks_.end() && it->second)
    {
        it->second(value);
    }
}

void managed_dialog::dispatch_grid_select(const element_def& elem, int32_t row, int32_t col)
{
    auto it = grid_callbacks_.find(elem.id);
    if (it != grid_callbacks_.end() && it->second)
    {
        it->second(row, col);
    }
}

void managed_dialog::dispatch_list_select(const element_def& elem, int32_t index)
{
    auto it = list_callbacks_.find(elem.id);
    if (it != list_callbacks_.end() && it->second)
    {
        it->second(index);
    }
}

} // namespace hb
