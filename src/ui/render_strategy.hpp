#pragma once

#include "ui/dialog_definition.hpp"
#include <memory>

namespace hb
{

class renderer;
class sprite_manager;

// Render mode selection
enum class render_mode
{
    modern, // Programmatic rendering with modern look
    classic // Sprite-based rendering matching original Helbreath
};

// Abstract strategy for rendering dialog elements
// Allows switching between classic (sprite-based) and modern (programmatic) rendering
class render_strategy
{
public:
    virtual ~render_strategy() = default;

    // Set the sprite manager (needed for classic rendering)
    virtual void set_sprite_manager(sprite_manager* sprites) { sprites_ = sprites; }
    sprite_manager* sprites() const { return sprites_; }

    // Render the dialog background
    virtual void render_background(renderer& rend, const dialog_definition& def, const ui_bounds& bounds) = 0;

    // Render the title bar
    virtual void
    render_title_bar(renderer& rend, const dialog_definition& def, const ui_bounds& bounds, bool close_hovered) = 0;

    // Render an individual element
    virtual void render_element(renderer& rend,
                                const element_def& elem,
                                const element_state& state,
                                const ui_bounds& absolute_bounds) = 0;

    // Get render mode
    virtual render_mode mode() const = 0;

protected:
    sprite_manager* sprites_ = nullptr;
};

// Modern (programmatic) rendering strategy
// Uses solid colors, gradients, and SFML primitives
class modern_render_strategy : public render_strategy
{
public:
    void render_background(renderer& rend, const dialog_definition& def, const ui_bounds& bounds) override;

    void render_title_bar(renderer& rend,
                          const dialog_definition& def,
                          const ui_bounds& bounds,
                          bool close_hovered) override;

    void render_element(renderer& rend,
                        const element_def& elem,
                        const element_state& state,
                        const ui_bounds& absolute_bounds) override;

    render_mode mode() const override { return render_mode::modern; }

private:
    void render_button(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void render_label(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void
    render_text_input(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void
    render_progress_bar(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void render_checkbox(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void render_slider(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void render_panel(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void render_separator(renderer& rend, const element_def& elem, const ui_bounds& bounds);
    void render_grid(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
};

// Classic (sprite-based) rendering strategy
// Uses original Helbreath PAK sprites
class classic_render_strategy : public render_strategy
{
public:
    void render_background(renderer& rend, const dialog_definition& def, const ui_bounds& bounds) override;

    void render_title_bar(renderer& rend,
                          const dialog_definition& def,
                          const ui_bounds& bounds,
                          bool close_hovered) override;

    void render_element(renderer& rend,
                        const element_def& elem,
                        const element_state& state,
                        const ui_bounds& absolute_bounds) override;

    render_mode mode() const override { return render_mode::classic; }

private:
    void
    render_sprite_button(renderer& rend, const element_def& elem, const element_state& state, const ui_bounds& bounds);
    void render_sprite_image(renderer& rend, const element_def& elem, const ui_bounds& bounds);

    // Fallback to modern when sprites not available
    modern_render_strategy fallback_;
};

// Factory function to create render strategy
std::unique_ptr<render_strategy> create_render_strategy(render_mode mode);

} // namespace hb
