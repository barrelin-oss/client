#pragma once

#include "ui/dialog_base.hpp"
#include "ui/dialog_definition.hpp"
#include "ui/render_strategy.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace hb
{

class sprite_manager;

// Data-driven dialog that can be configured from dialog_definition
// Supports both callback-based simple dialogs and subclassing for complex dialogs
class managed_dialog : public dialog
{
public:
    explicit managed_dialog(dialog_definition def);
    ~managed_dialog() override = default;

    // === Event callbacks (for simple dialogs) ===

    // Button click callback
    void on_button_click(std::string_view element_id, std::function<void()> callback);

    // Checkbox change callback
    void on_checkbox_change(std::string_view element_id, std::function<void(bool)> callback);

    // Text input change callback
    void on_text_change(std::string_view element_id, std::function<void(std::string_view)> callback);

    // Slider value change callback
    void on_slider_change(std::string_view element_id, std::function<void(float)> callback);

    // Grid cell selection callback (row, col)
    void on_grid_select(std::string_view element_id, std::function<void(int32_t, int32_t)> callback);

    // List item selection callback
    void on_list_select(std::string_view element_id, std::function<void(int32_t)> callback);

    // Action handler for YAML-based actions (set by dialog_manager)
    using action_handler = std::function<void(element_action action, std::string_view target)>;
    void set_action_handler(action_handler handler);

    // === Element access ===

    // Labels
    void set_label_text(std::string_view element_id, std::string_view text);

    // Progress bars
    void set_progress(std::string_view element_id, float value); // 0.0-1.0

    // Checkboxes
    void set_checkbox_checked(std::string_view element_id, bool checked);
    bool get_checkbox_checked(std::string_view element_id) const;

    // Text inputs
    void set_text_input(std::string_view element_id, std::string_view text);
    std::string_view get_text_input(std::string_view element_id) const;

    // Sliders
    void set_slider_value(std::string_view element_id, float value);
    float get_slider_value(std::string_view element_id) const;

    // Visibility and enabled state
    void set_element_visible(std::string_view element_id, bool visible);
    void set_element_enabled(std::string_view element_id, bool enabled);
    bool is_element_visible(std::string_view element_id) const;
    bool is_element_enabled(std::string_view element_id) const;

    // Grid/List selection
    void set_selected_index(std::string_view element_id, int32_t index);
    int32_t get_selected_index(std::string_view element_id) const;

    // Button text
    void set_button_text(std::string_view element_id, std::string_view text);

    // === Rendering mode ===

    void set_render_strategy(std::unique_ptr<render_strategy> strategy);
    void set_render_mode(render_mode mode);
    render_mode get_render_mode() const;

    // Sprite manager for classic rendering
    void set_sprite_manager(sprite_manager* sprites);
    sprite_manager* sprites() const { return sprites_; }

    // === dialog overrides ===

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;
    bool handle_key_press(sf::Keyboard::Key key) override;
    bool handle_text_input(char32_t unicode) override;
    bool handle_mouse_wheel(int32_t x, int32_t y, int32_t delta);

    // === Definition access ===

    const dialog_definition& definition() const { return definition_; }

    // Update definition in place (for hot-reload without losing state)
    void update_definition(const dialog_definition& new_def);

    // Open/close with hooks
    void open();
    void close();

protected:
    // === Override points for subclassing (complex dialogs) ===

    // Called when dialog is opened
    virtual void on_open_impl() {}

    // Called when dialog is closed
    virtual void on_close_impl() {}

    // Called every frame
    virtual void on_update_impl([[maybe_unused]] float delta_time) {}

    // Custom rendering - return true if you handled rendering, false to use default
    virtual bool on_custom_render([[maybe_unused]] renderer& rend) { return false; }

    // Custom input handling - return true if consumed
    virtual bool
    on_custom_mouse_down([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y, [[maybe_unused]] sf::Mouse::Button btn)
    {
        return false;
    }
    virtual bool
    on_custom_mouse_up([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y, [[maybe_unused]] sf::Mouse::Button btn)
    {
        return false;
    }
    virtual bool on_custom_mouse_move([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y) { return false; }
    virtual bool on_custom_key_press([[maybe_unused]] sf::Keyboard::Key key) { return false; }
    virtual bool
    on_custom_mouse_wheel([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y, [[maybe_unused]] int32_t delta)
    {
        return false;
    }

    // Access to element states for subclasses
    element_state& get_element_state(std::string_view id);
    const element_state& get_element_state(std::string_view id) const;

    // Get absolute bounds for an element (for subclass custom rendering)
    ui_bounds get_absolute_bounds(const element_def& elem) const;

    // Element states keyed by element ID (accessible to subclasses)
    std::map<std::string, element_state, std::less<>> element_states_;

    // Sprite manager for subclass rendering
    sprite_manager* sprites_ = nullptr;

private:
    // Find element definition by ID
    const element_def* find_element_def(std::string_view id) const;

    // Hit testing
    const element_def* element_at_point(int32_t x, int32_t y) const;
    std::pair<int32_t, int32_t> grid_cell_at_point(const element_def& elem, int32_t x, int32_t y) const;

    // Internal event dispatching
    void dispatch_button_click(const element_def& elem);
    void dispatch_checkbox_change(const element_def& elem, bool checked);
    void dispatch_text_change(const element_def& elem, std::string_view text);
    void dispatch_slider_change(const element_def& elem, float value);
    void dispatch_grid_select(const element_def& elem, int32_t row, int32_t col);
    void dispatch_list_select(const element_def& elem, int32_t index);

    dialog_definition definition_;
    std::unique_ptr<render_strategy> render_strategy_;

    // Callbacks keyed by element ID
    std::map<std::string, std::function<void()>, std::less<>> button_callbacks_;
    std::map<std::string, std::function<void(bool)>, std::less<>> checkbox_callbacks_;
    std::map<std::string, std::function<void(std::string_view)>, std::less<>> text_callbacks_;
    std::map<std::string, std::function<void(float)>, std::less<>> slider_callbacks_;
    std::map<std::string, std::function<void(int32_t, int32_t)>, std::less<>> grid_callbacks_;
    std::map<std::string, std::function<void(int32_t)>, std::less<>> list_callbacks_;
    action_handler action_handler_;

    // UI state
    std::string hovered_element_id_;
    std::string pressed_element_id_;
    std::string focused_element_id_;
    bool close_button_hovered_ = false;

    // Dragging state (inherited from base class but we handle it here too)
    bool is_dragging_ = false;
    int32_t drag_start_x_ = 0;
    int32_t drag_start_y_ = 0;

    static constexpr int32_t title_bar_height = 24;
};

} // namespace hb
