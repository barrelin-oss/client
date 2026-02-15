#pragma once

#include "ui/dialog_definition.hpp"
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string_view>

namespace hb
{

// Fluent builder API for creating dialog definitions
// Usage:
//   auto def = dialog_builder::create("my_dialog")
//       .title("My Dialog")
//       .bounds(100, 100, 300, 200)
//       .modal(true)
//       .label("lbl_message", 20, 30, "Hello World")
//       .button("btn_ok", 110, 150, 80, 28, "OK")
//       .build();
class dialog_builder
{
public:
    // Start building a new dialog definition
    static dialog_builder create(std::string_view id);

    // === Dialog properties ===

    dialog_builder& title(std::string_view title);
    dialog_builder& bounds(int32_t x, int32_t y, int32_t w, int32_t h);
    dialog_builder& size(int32_t w, int32_t h); // Position will be centered
    dialog_builder& modal(bool m = true);
    dialog_builder& draggable(bool d = true);
    dialog_builder& closeable(bool c = true);
    dialog_builder& has_border(bool b = true);
    dialog_builder& has_title_bar(bool t = true);
    dialog_builder& centered(bool c = true);

    // Visual styling
    dialog_builder& background(sf::Color color);
    dialog_builder& border_color(sf::Color color);
    dialog_builder& title_bar_color(sf::Color color);
    dialog_builder& title_text_color(sf::Color color);

    // Classic sprite-based background
    dialog_builder& background_sprite(std::string_view pak, int32_t idx, int32_t frame = 0);

    // === Element creation methods ===

    // Label (static text)
    dialog_builder& label(std::string_view id,
                          int32_t x,
                          int32_t y,
                          std::string_view text,
                          sf::Color color = sf::Color::White,
                          uint32_t font_size = 12);

    // Button
    dialog_builder& button(std::string_view id, int32_t x, int32_t y, int32_t w, int32_t h, std::string_view text);

    dialog_builder& button_styled(std::string_view id,
                                  int32_t x,
                                  int32_t y,
                                  int32_t w,
                                  int32_t h,
                                  std::string_view text,
                                  sf::Color bg,
                                  sf::Color hover,
                                  sf::Color pressed);

    // Sprite button (for classic UI)
    dialog_builder& sprite_button(std::string_view id,
                                  int32_t x,
                                  int32_t y,
                                  std::string_view pak,
                                  int32_t sprite_idx,
                                  int32_t normal_frame,
                                  int32_t hover_frame = -1,
                                  int32_t pressed_frame = -1);

    // Text input
    dialog_builder& text_input(
        std::string_view id, int32_t x, int32_t y, int32_t w, int32_t h, int32_t max_chars = 32, bool password = false);

    // Image (static sprite)
    dialog_builder&
    image(std::string_view id, int32_t x, int32_t y, std::string_view pak, int32_t sprite_idx, int32_t frame = 0);

    // Progress bar
    dialog_builder& progress_bar(std::string_view id,
                                 int32_t x,
                                 int32_t y,
                                 int32_t w,
                                 int32_t h,
                                 sf::Color fill = sf::Color(100, 200, 100),
                                 sf::Color bg = sf::Color(40, 40, 50));

    // Checkbox
    dialog_builder& checkbox(std::string_view id, int32_t x, int32_t y, std::string_view label_text);

    // Slider
    dialog_builder& slider(std::string_view id,
                           int32_t x,
                           int32_t y,
                           int32_t w,
                           int32_t h,
                           float min_val = 0.0f,
                           float max_val = 1.0f,
                           float step = 0.01f);

    // List box
    dialog_builder& list_box(std::string_view id, int32_t x, int32_t y, int32_t w, int32_t h);

    // Grid (for inventory-style layouts)
    dialog_builder& grid(std::string_view id,
                         int32_t x,
                         int32_t y,
                         int32_t cols,
                         int32_t rows,
                         int32_t cell_w,
                         int32_t cell_h,
                         int32_t padding = 2);

    // Panel (container)
    dialog_builder&
    panel(std::string_view id, int32_t x, int32_t y, int32_t w, int32_t h, sf::Color bg = sf::Color(40, 40, 50, 200));

    // Separator line
    dialog_builder& separator(int32_t x, int32_t y, int32_t length, bool horizontal = true);

    // Raw sprite (for custom graphics)
    dialog_builder&
    sprite(std::string_view id, int32_t x, int32_t y, std::string_view pak, int32_t sprite_idx, int32_t frame = 0);

    // === Element modification (applies to last added element) ===

    // Add tooltip to last element
    dialog_builder& tooltip(std::string_view text);

    // Set custom property on last element
    dialog_builder& property(std::string_view key, std::string_view value);

    // === Build ===

    dialog_definition build();

private:
    explicit dialog_builder(std::string_view id);

    dialog_definition def_;
    element_def* last_element_ = nullptr;

    // Helper to add element and track it
    element_def& add_element(element_type type, std::string_view id);
};

} // namespace hb
