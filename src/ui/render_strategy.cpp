#include "ui/render_strategy.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include <algorithm>
#include <cmath>
#include <format>

namespace hb {

std::unique_ptr<render_strategy> create_render_strategy(render_mode mode) {
    switch (mode) {
        case render_mode::classic:
            return std::make_unique<classic_render_strategy>();
        case render_mode::modern:
        default:
            return std::make_unique<modern_render_strategy>();
    }
}

// =============================================================================
// Modern Render Strategy
// =============================================================================

void modern_render_strategy::render_background(renderer& rend,
                                                const dialog_definition& def,
                                                const ui_bounds& bounds) {
    sf::Color bg = def.background_color.value_or(sf::Color(32, 32, 48, 240));

    // Draw background
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, bg, true);

    // Draw border if enabled
    if (def.has_border) {
        sf::Color border = def.border_color.value_or(sf::Color(80, 80, 100));
        rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, border, false);

        // Inner highlight
        rend.draw_line(bounds.x + 1, bounds.y + 1,
                       bounds.x + bounds.width - 2, bounds.y + 1,
                       sf::Color(100, 100, 120, 100));
        rend.draw_line(bounds.x + 1, bounds.y + 1,
                       bounds.x + 1, bounds.y + bounds.height - 2,
                       sf::Color(100, 100, 120, 100));
    }
}

void modern_render_strategy::render_title_bar(renderer& rend,
                                               const dialog_definition& def,
                                               const ui_bounds& bounds,
                                               bool close_hovered) {
    if (!def.has_title_bar) return;

    constexpr int32_t title_height = 24;

    sf::Color title_bg = def.title_bar_color.value_or(sf::Color(50, 50, 70));
    sf::Color title_text = def.title_text_color.value_or(sf::Color::White);

    // Title bar background
    rend.draw_rect(bounds.x, bounds.y, bounds.width, title_height, title_bg, true);

    // Title bar bottom border
    rend.draw_line(bounds.x, bounds.y + title_height,
                   bounds.x + bounds.width, bounds.y + title_height,
                   sf::Color(80, 80, 100));

    // Title text
    if (!def.title.empty()) {
        rend.draw_text(def.title, bounds.x + 8, bounds.y + 5, title_text, 11);
    }

    // Close button
    if (def.closeable) {
        int32_t close_x = bounds.x + bounds.width - 22;
        int32_t close_y = bounds.y + 4;
        int32_t close_size = 16;

        sf::Color close_bg = close_hovered ?
            sf::Color(180, 60, 60) : sf::Color(120, 60, 60);

        rend.draw_rect(close_x, close_y, close_size, close_size, close_bg, true);
        rend.draw_rect(close_x, close_y, close_size, close_size,
                       sf::Color(150, 100, 100), false);

        // X symbol
        rend.draw_text("X", close_x + 4, close_y + 1, sf::Color::White, 10);
    }
}

void modern_render_strategy::render_element(renderer& rend,
                                             const element_def& elem,
                                             const element_state& state,
                                             const ui_bounds& absolute_bounds) {
    if (!state.visible) return;

    switch (elem.type) {
        case element_type::label:
            render_label(rend, elem, state, absolute_bounds);
            break;
        case element_type::button:
            render_button(rend, elem, state, absolute_bounds);
            break;
        case element_type::text_input:
            render_text_input(rend, elem, state, absolute_bounds);
            break;
        case element_type::progress_bar:
            render_progress_bar(rend, elem, state, absolute_bounds);
            break;
        case element_type::checkbox:
            render_checkbox(rend, elem, state, absolute_bounds);
            break;
        case element_type::slider:
            render_slider(rend, elem, state, absolute_bounds);
            break;
        case element_type::panel:
            render_panel(rend, elem, state, absolute_bounds);
            break;
        case element_type::separator:
            render_separator(rend, elem, absolute_bounds);
            break;
        case element_type::grid:
            render_grid(rend, elem, state, absolute_bounds);
            break;
        case element_type::image:
        case element_type::sprite:
            // Sprites require sprite_manager - handle in classic strategy
            break;
        default:
            break;
    }
}

void modern_render_strategy::render_button(renderer& rend, const element_def& elem,
                                            const element_state& state,
                                            const ui_bounds& bounds) {
    sf::Color bg;
    if (!state.enabled) {
        bg = elem.disabled_color.value_or(sf::Color(50, 50, 50));
    } else if (state.pressed) {
        bg = elem.pressed_color.value_or(sf::Color(40, 40, 60));
    } else if (state.hovered) {
        bg = elem.hover_color.value_or(sf::Color(70, 70, 90));
    } else {
        bg = elem.background_color.value_or(sf::Color(55, 55, 75));
    }

    // Background
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, bg, true);

    // Border with bevel effect
    if (state.pressed) {
        rend.draw_line(bounds.x, bounds.y, bounds.x + bounds.width - 1, bounds.y,
                       sf::Color(30, 30, 40));
        rend.draw_line(bounds.x, bounds.y, bounds.x, bounds.y + bounds.height - 1,
                       sf::Color(30, 30, 40));
        rend.draw_line(bounds.x + bounds.width - 1, bounds.y,
                       bounds.x + bounds.width - 1, bounds.y + bounds.height - 1,
                       sf::Color(100, 100, 120));
        rend.draw_line(bounds.x, bounds.y + bounds.height - 1,
                       bounds.x + bounds.width - 1, bounds.y + bounds.height - 1,
                       sf::Color(100, 100, 120));
    } else {
        rend.draw_line(bounds.x, bounds.y, bounds.x + bounds.width - 1, bounds.y,
                       sf::Color(100, 100, 120));
        rend.draw_line(bounds.x, bounds.y, bounds.x, bounds.y + bounds.height - 1,
                       sf::Color(100, 100, 120));
        rend.draw_line(bounds.x + bounds.width - 1, bounds.y,
                       bounds.x + bounds.width - 1, bounds.y + bounds.height - 1,
                       sf::Color(30, 30, 40));
        rend.draw_line(bounds.x, bounds.y + bounds.height - 1,
                       bounds.x + bounds.width - 1, bounds.y + bounds.height - 1,
                       sf::Color(30, 30, 40));
    }

    // Text
    std::string_view text = state.text.empty() ? std::string_view(elem.text) : state.text;
    if (!text.empty()) {
        sf::Color text_color = elem.text_color.value_or(sf::Color::White);
        if (!state.enabled) {
            text_color = sf::Color(120, 120, 120);
        }

        // Center text
        int32_t text_width = static_cast<int32_t>(text.length()) * 6;
        int32_t text_x = bounds.x + (bounds.width - text_width) / 2;
        int32_t text_y = bounds.y + (bounds.height - 12) / 2;

        // Offset when pressed
        if (state.pressed) {
            text_x += 1;
            text_y += 1;
        }

        rend.draw_text(text, text_x, text_y, text_color,
                       elem.font_size.value_or(11));
    }
}

void modern_render_strategy::render_label(renderer& rend, const element_def& elem,
                                           const element_state& state,
                                           const ui_bounds& bounds) {
    std::string_view text = state.text.empty() ? std::string_view(elem.text) : state.text;
    if (text.empty()) return;

    sf::Color color = elem.text_color.value_or(sf::Color::White);
    rend.draw_text(text, bounds.x, bounds.y, color,
                   elem.font_size.value_or(11));
}

void modern_render_strategy::render_text_input(renderer& rend, const element_def& elem,
                                                const element_state& state,
                                                const ui_bounds& bounds) {
    sf::Color bg = elem.background_color.value_or(sf::Color(25, 25, 35));
    sf::Color border = state.focused ?
        sf::Color(100, 140, 200) :
        elem.border_color.value_or(sf::Color(60, 60, 80));

    // Background
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, bg, true);
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, border, false);

    // Text
    std::string display_text = state.text;
    if (elem.password_mode && !display_text.empty()) {
        display_text = std::string(display_text.length(), '*');
    }

    sf::Color text_color = elem.text_color.value_or(sf::Color::White);
    int32_t text_x = bounds.x + 4;
    int32_t text_y = bounds.y + (bounds.height - 12) / 2;

    if (!display_text.empty()) {
        rend.draw_text(display_text, text_x, text_y, text_color, 11);
    }

    // Cursor when focused
    if (state.focused) {
        int32_t cursor_x = text_x + static_cast<int32_t>(display_text.length()) * 6;
        rend.draw_rect(cursor_x, bounds.y + 4, 1, bounds.height - 8,
                       sf::Color::White, true);
    }
}

void modern_render_strategy::render_progress_bar(renderer& rend, const element_def& elem,
                                                  const element_state& state,
                                                  const ui_bounds& bounds) {
    sf::Color bg = elem.background_color.value_or(sf::Color(30, 30, 40));
    sf::Color fill = elem.fill_color.value_or(sf::Color(80, 180, 80));
    sf::Color border = elem.border_color.value_or(sf::Color(60, 60, 80));

    // Background
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, bg, true);

    // Fill
    float progress = std::clamp(state.progress, 0.0f, 1.0f);
    int32_t fill_width = static_cast<int32_t>(bounds.width * progress);
    if (fill_width > 0) {
        rend.draw_rect(bounds.x, bounds.y, fill_width, bounds.height, fill, true);

        // Highlight
        sf::Color highlight(
            static_cast<uint8_t>(std::min(255, fill.r + 40)),
            static_cast<uint8_t>(std::min(255, fill.g + 40)),
            static_cast<uint8_t>(std::min(255, fill.b + 40))
        );
        rend.draw_rect(bounds.x, bounds.y, fill_width, 2, highlight, true);
    }

    // Border
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, border, false);

    // Value text
    if (elem.show_value_text) {
        std::string value_str = std::format("{}%", static_cast<int>(progress * 100));
        int32_t text_width = static_cast<int32_t>(value_str.length()) * 6;
        int32_t text_x = bounds.x + (bounds.width - text_width) / 2;
        int32_t text_y = bounds.y + (bounds.height - 10) / 2;
        rend.draw_text(value_str, text_x, text_y, sf::Color::White, 10);
    }
}

void modern_render_strategy::render_checkbox(renderer& rend, const element_def& elem,
                                              const element_state& state,
                                              const ui_bounds& bounds) {
    constexpr int32_t box_size = 16;

    sf::Color bg = elem.background_color.value_or(sf::Color(35, 35, 45));
    sf::Color border = state.hovered ?
        sf::Color(100, 140, 200) :
        elem.border_color.value_or(sf::Color(70, 70, 90));

    // Checkbox box
    rend.draw_rect(bounds.x, bounds.y, box_size, box_size, bg, true);
    rend.draw_rect(bounds.x, bounds.y, box_size, box_size, border, false);

    // Checkmark when checked
    if (state.checked) {
        sf::Color check_color = sf::Color(100, 200, 100);
        rend.draw_text("v", bounds.x + 3, bounds.y + 1, check_color, 12);
    }

    // Label text
    std::string_view label = elem.text;
    if (!label.empty()) {
        sf::Color text_color = elem.text_color.value_or(sf::Color::White);
        rend.draw_text(label, bounds.x + box_size + 6, bounds.y + 2, text_color, 11);
    }
}

void modern_render_strategy::render_slider(renderer& rend, const element_def& elem,
                                            const element_state& state,
                                            const ui_bounds& bounds) {
    sf::Color bg = elem.background_color.value_or(sf::Color(30, 30, 40));
    sf::Color fill = elem.fill_color.value_or(sf::Color(70, 120, 180));
    sf::Color border = elem.border_color.value_or(sf::Color(60, 60, 80));

    // Track background
    int32_t track_y = bounds.y + bounds.height / 2 - 3;
    rend.draw_rect(bounds.x, track_y, bounds.width, 6, bg, true);
    rend.draw_rect(bounds.x, track_y, bounds.width, 6, border, false);

    // Filled portion
    float value = std::clamp(state.slider_value, 0.0f, 1.0f);
    int32_t fill_width = static_cast<int32_t>(bounds.width * value);
    if (fill_width > 0) {
        rend.draw_rect(bounds.x, track_y, fill_width, 6, fill, true);
    }

    // Thumb
    int32_t thumb_x = bounds.x + fill_width - 6;
    int32_t thumb_y = bounds.y + bounds.height / 2 - 8;
    sf::Color thumb_color = state.pressed ?
        sf::Color(140, 180, 220) : sf::Color(100, 150, 200);
    rend.draw_rect(thumb_x, thumb_y, 12, 16, thumb_color, true);
    rend.draw_rect(thumb_x, thumb_y, 12, 16, sf::Color(150, 180, 220), false);
}

void modern_render_strategy::render_panel(renderer& rend, const element_def& elem,
                                           const element_state& state,
                                           const ui_bounds& bounds) {
    sf::Color bg = elem.background_color.value_or(sf::Color(35, 35, 50, 200));
    sf::Color border = elem.border_color.value_or(sf::Color(60, 60, 80));

    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, bg, true);
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, border, false);
}

void modern_render_strategy::render_separator(renderer& rend, const element_def& elem,
                                               const ui_bounds& bounds) {
    sf::Color color = elem.border_color.value_or(sf::Color(70, 70, 90));

    if (bounds.width > bounds.height) {
        // Horizontal separator
        rend.draw_line(bounds.x, bounds.y, bounds.x + bounds.width, bounds.y, color);
    } else {
        // Vertical separator
        rend.draw_line(bounds.x, bounds.y, bounds.x, bounds.y + bounds.height, color);
    }
}

void modern_render_strategy::render_grid(renderer& rend, const element_def& elem,
                                          const element_state& state,
                                          const ui_bounds& bounds) {
    sf::Color bg = elem.background_color.value_or(sf::Color(30, 30, 40));
    sf::Color border = elem.border_color.value_or(sf::Color(50, 50, 70));
    sf::Color cell_bg = sf::Color(40, 40, 55);
    sf::Color hover_color = sf::Color(60, 60, 80);

    // Draw overall background
    rend.draw_rect(bounds.x, bounds.y, bounds.width, bounds.height, bg, true);

    // Draw cells
    for (int32_t row = 0; row < elem.grid_rows; ++row) {
        for (int32_t col = 0; col < elem.grid_cols; ++col) {
            int32_t cell_x = bounds.x + col * (elem.cell_width + elem.cell_padding);
            int32_t cell_y = bounds.y + row * (elem.cell_height + elem.cell_padding);

            int32_t cell_index = row * elem.grid_cols + col;
            bool is_selected = (cell_index == state.selected_index);

            sf::Color bg_color = is_selected ? hover_color : cell_bg;
            rend.draw_rect(cell_x, cell_y, elem.cell_width, elem.cell_height, bg_color, true);
            rend.draw_rect(cell_x, cell_y, elem.cell_width, elem.cell_height, border, false);
        }
    }
}

// =============================================================================
// Classic Render Strategy
// =============================================================================

void classic_render_strategy::render_background(renderer& rend,
                                                 const dialog_definition& def,
                                                 const ui_bounds& bounds) {
    // Try to render sprite background
    if (sprites_ && !def.background_sprite_pak.empty()) {
        const sprite* spr = sprites_->get_sprite(def.background_sprite_pak,
                                                  def.background_sprite_index);
        if (spr && spr->frame_count() > static_cast<uint32_t>(def.background_sprite_frame)) {
            spr->draw(rend.window(), bounds.x, bounds.y, def.background_sprite_frame);
            return;
        }
    }

    // Fallback to modern rendering
    fallback_.render_background(rend, def, bounds);
}

void classic_render_strategy::render_title_bar(renderer& rend,
                                                const dialog_definition& def,
                                                const ui_bounds& bounds,
                                                bool close_hovered) {
    // Classic Helbreath dialogs typically have the title bar integrated into
    // the background sprite, so we often don't need to draw it separately.
    // But we still need the close button functionality.

    if (!def.has_title_bar) return;

    // If there's a background sprite, assume title is part of it
    if (!def.background_sprite_pak.empty()) {
        // Just draw close button if closeable
        if (def.closeable) {
            constexpr int32_t close_size = 16;
            int32_t close_x = bounds.x + bounds.width - close_size - 6;
            int32_t close_y = bounds.y + 4;

            sf::Color close_bg = close_hovered ?
                sf::Color(180, 60, 60) : sf::Color(120, 60, 60);

            rend.draw_rect(close_x, close_y, close_size, close_size, close_bg, true);
            rend.draw_text("X", close_x + 4, close_y + 1, sf::Color::White, 10);
        }
        return;
    }

    // Fallback to modern title bar
    fallback_.render_title_bar(rend, def, bounds, close_hovered);
}

void classic_render_strategy::render_element(renderer& rend,
                                              const element_def& elem,
                                              const element_state& state,
                                              const ui_bounds& absolute_bounds) {
    if (!state.visible) return;

    // Handle sprite-based elements
    if (!elem.sprite_pak.empty()) {
        switch (elem.type) {
            case element_type::button:
                render_sprite_button(rend, elem, state, absolute_bounds);
                return;
            case element_type::image:
            case element_type::sprite:
                render_sprite_image(rend, elem, absolute_bounds);
                return;
            default:
                break;
        }
    }

    // Fall back to modern rendering for non-sprite elements
    fallback_.render_element(rend, elem, state, absolute_bounds);
}

void classic_render_strategy::render_sprite_button(renderer& rend, const element_def& elem,
                                                    const element_state& state,
                                                    const ui_bounds& bounds) {
    if (!sprites_) {
        fallback_.render_element(rend, elem, state, bounds);
        return;
    }

    const sprite* spr = sprites_->get_sprite(elem.sprite_pak, elem.sprite_index);
    if (!spr) {
        fallback_.render_element(rend, elem, state, bounds);
        return;
    }

    // Determine which frame to use based on state
    int32_t frame = elem.sprite_frame;

    if (state.pressed) {
        auto it = elem.properties.find("pressed_frame");
        if (it != elem.properties.end()) {
            frame = std::stoi(it->second);
        }
    } else if (state.hovered) {
        auto it = elem.properties.find("hover_frame");
        if (it != elem.properties.end()) {
            frame = std::stoi(it->second);
        }
    }

    if (static_cast<uint32_t>(frame) < spr->frame_count()) {
        spr->draw(rend.window(), bounds.x, bounds.y, frame);
    }
}

void classic_render_strategy::render_sprite_image(renderer& rend, const element_def& elem,
                                                   const ui_bounds& bounds) {
    if (!sprites_) return;

    const sprite* spr = sprites_->get_sprite(elem.sprite_pak, elem.sprite_index);
    if (!spr) return;

    if (static_cast<uint32_t>(elem.sprite_frame) < spr->frame_count()) {
        spr->draw(rend.window(), bounds.x, bounds.y, elem.sprite_frame);
    }
}

} // namespace hb
