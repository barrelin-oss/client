#include "ui/dialog_builder.hpp"
#include "core/constants.hpp"

namespace hb {

dialog_builder dialog_builder::create(std::string_view id) {
    return dialog_builder(id);
}

dialog_builder::dialog_builder(std::string_view id) {
    def_.id = id;
    def_.title = "";
    def_.bounds = {0, 0, 200, 150};
}

// === Dialog properties ===

dialog_builder& dialog_builder::title(std::string_view title) {
    def_.title = title;
    return *this;
}

dialog_builder& dialog_builder::bounds(int32_t x, int32_t y, int32_t w, int32_t h) {
    def_.bounds = {x, y, w, h};
    return *this;
}

dialog_builder& dialog_builder::size(int32_t w, int32_t h) {
    def_.bounds.width = w;
    def_.bounds.height = h;
    def_.centered = true;
    return *this;
}

dialog_builder& dialog_builder::modal(bool m) {
    def_.modal = m;
    return *this;
}

dialog_builder& dialog_builder::draggable(bool d) {
    def_.draggable = d;
    return *this;
}

dialog_builder& dialog_builder::closeable(bool c) {
    def_.closeable = c;
    return *this;
}

dialog_builder& dialog_builder::has_border(bool b) {
    def_.has_border = b;
    return *this;
}

dialog_builder& dialog_builder::has_title_bar(bool t) {
    def_.has_title_bar = t;
    return *this;
}

dialog_builder& dialog_builder::centered(bool c) {
    def_.centered = c;
    return *this;
}

dialog_builder& dialog_builder::background(sf::Color color) {
    def_.background_color = color;
    return *this;
}

dialog_builder& dialog_builder::border_color(sf::Color color) {
    def_.border_color = color;
    return *this;
}

dialog_builder& dialog_builder::title_bar_color(sf::Color color) {
    def_.title_bar_color = color;
    return *this;
}

dialog_builder& dialog_builder::title_text_color(sf::Color color) {
    def_.title_text_color = color;
    return *this;
}

dialog_builder& dialog_builder::background_sprite(std::string_view pak, int32_t idx, int32_t frame) {
    def_.background_sprite_pak = pak;
    def_.background_sprite_index = idx;
    def_.background_sprite_frame = frame;
    return *this;
}

// === Element creation ===

element_def& dialog_builder::add_element(element_type type, std::string_view id) {
    element_def elem;
    elem.id = id;
    elem.type = type;
    def_.elements.push_back(std::move(elem));
    last_element_ = &def_.elements.back();
    return *last_element_;
}

dialog_builder& dialog_builder::label(std::string_view id, int32_t x, int32_t y,
                                       std::string_view text,
                                       sf::Color color, uint32_t font_size) {
    auto& elem = add_element(element_type::label, id);
    elem.bounds = {x, y, 0, 0};  // Size determined by text
    elem.text = text;
    elem.text_color = color;
    elem.font_size = font_size;
    return *this;
}

dialog_builder& dialog_builder::button(std::string_view id, int32_t x, int32_t y,
                                        int32_t w, int32_t h,
                                        std::string_view text) {
    auto& elem = add_element(element_type::button, id);
    elem.bounds = {x, y, w, h};
    elem.text = text;
    elem.background_color = sf::Color(60, 60, 80);
    elem.hover_color = sf::Color(80, 80, 100);
    elem.pressed_color = sf::Color(50, 50, 70);
    elem.text_color = sf::Color::White;
    return *this;
}

dialog_builder& dialog_builder::button_styled(std::string_view id, int32_t x, int32_t y,
                                               int32_t w, int32_t h,
                                               std::string_view text,
                                               sf::Color bg, sf::Color hover, sf::Color pressed) {
    auto& elem = add_element(element_type::button, id);
    elem.bounds = {x, y, w, h};
    elem.text = text;
    elem.background_color = bg;
    elem.hover_color = hover;
    elem.pressed_color = pressed;
    elem.text_color = sf::Color::White;
    return *this;
}

dialog_builder& dialog_builder::sprite_button(std::string_view id, int32_t x, int32_t y,
                                               std::string_view pak, int32_t sprite_idx,
                                               int32_t normal_frame, int32_t hover_frame,
                                               int32_t pressed_frame) {
    auto& elem = add_element(element_type::button, id);
    elem.bounds = {x, y, 0, 0};  // Size determined by sprite
    elem.sprite_pak = pak;
    elem.sprite_index = sprite_idx;
    elem.sprite_frame = normal_frame;

    // Store hover/pressed frames in properties
    if (hover_frame >= 0) {
        elem.properties["hover_frame"] = std::to_string(hover_frame);
    }
    if (pressed_frame >= 0) {
        elem.properties["pressed_frame"] = std::to_string(pressed_frame);
    }
    return *this;
}

dialog_builder& dialog_builder::text_input(std::string_view id, int32_t x, int32_t y,
                                            int32_t w, int32_t h,
                                            int32_t max_chars, bool password) {
    auto& elem = add_element(element_type::text_input, id);
    elem.bounds = {x, y, w, h};
    elem.max_chars = max_chars;
    elem.password_mode = password;
    elem.background_color = sf::Color(30, 30, 40);
    elem.border_color = sf::Color(80, 80, 100);
    elem.text_color = sf::Color::White;
    return *this;
}

dialog_builder& dialog_builder::image(std::string_view id, int32_t x, int32_t y,
                                       std::string_view pak, int32_t sprite_idx, int32_t frame) {
    auto& elem = add_element(element_type::image, id);
    elem.bounds = {x, y, 0, 0};  // Size determined by sprite
    elem.sprite_pak = pak;
    elem.sprite_index = sprite_idx;
    elem.sprite_frame = frame;
    return *this;
}

dialog_builder& dialog_builder::progress_bar(std::string_view id, int32_t x, int32_t y,
                                              int32_t w, int32_t h,
                                              sf::Color fill, sf::Color bg) {
    auto& elem = add_element(element_type::progress_bar, id);
    elem.bounds = {x, y, w, h};
    elem.fill_color = fill;
    elem.background_color = bg;
    elem.border_color = sf::Color(80, 80, 100);
    return *this;
}

dialog_builder& dialog_builder::checkbox(std::string_view id, int32_t x, int32_t y,
                                          std::string_view label_text) {
    auto& elem = add_element(element_type::checkbox, id);
    elem.bounds = {x, y, 16, 16};  // Checkbox size
    elem.text = label_text;
    elem.text_color = sf::Color::White;
    elem.background_color = sf::Color(40, 40, 50);
    elem.border_color = sf::Color(80, 80, 100);
    return *this;
}

dialog_builder& dialog_builder::slider(std::string_view id, int32_t x, int32_t y,
                                        int32_t w, int32_t h,
                                        float min_val, float max_val, float step) {
    auto& elem = add_element(element_type::slider, id);
    elem.bounds = {x, y, w, h};
    elem.min_value = min_val;
    elem.max_value = max_val;
    elem.step = step;
    elem.background_color = sf::Color(40, 40, 50);
    elem.fill_color = sf::Color(80, 120, 180);
    elem.border_color = sf::Color(80, 80, 100);
    return *this;
}

dialog_builder& dialog_builder::list_box(std::string_view id, int32_t x, int32_t y,
                                          int32_t w, int32_t h) {
    auto& elem = add_element(element_type::list_box, id);
    elem.bounds = {x, y, w, h};
    elem.background_color = sf::Color(30, 30, 40);
    elem.border_color = sf::Color(80, 80, 100);
    elem.text_color = sf::Color::White;
    return *this;
}

dialog_builder& dialog_builder::grid(std::string_view id, int32_t x, int32_t y,
                                      int32_t cols, int32_t rows,
                                      int32_t cell_w, int32_t cell_h, int32_t padding) {
    auto& elem = add_element(element_type::grid, id);
    int32_t total_w = cols * (cell_w + padding) - padding;
    int32_t total_h = rows * (cell_h + padding) - padding;
    elem.bounds = {x, y, total_w, total_h};
    elem.grid_cols = cols;
    elem.grid_rows = rows;
    elem.cell_width = cell_w;
    elem.cell_height = cell_h;
    elem.cell_padding = padding;
    elem.background_color = sf::Color(30, 30, 40);
    elem.border_color = sf::Color(60, 60, 80);
    return *this;
}

dialog_builder& dialog_builder::panel(std::string_view id, int32_t x, int32_t y,
                                       int32_t w, int32_t h, sf::Color bg) {
    auto& elem = add_element(element_type::panel, id);
    elem.bounds = {x, y, w, h};
    elem.background_color = bg;
    elem.border_color = sf::Color(80, 80, 100);
    return *this;
}

dialog_builder& dialog_builder::separator(int32_t x, int32_t y, int32_t length, bool horizontal) {
    static int separator_count = 0;
    std::string id = "sep_" + std::to_string(separator_count++);

    auto& elem = add_element(element_type::separator, id);
    if (horizontal) {
        elem.bounds = {x, y, length, 1};
    } else {
        elem.bounds = {x, y, 1, length};
    }
    elem.border_color = sf::Color(80, 80, 100);
    return *this;
}

dialog_builder& dialog_builder::sprite(std::string_view id, int32_t x, int32_t y,
                                        std::string_view pak, int32_t sprite_idx, int32_t frame) {
    auto& elem = add_element(element_type::sprite, id);
    elem.bounds = {x, y, 0, 0};  // Size determined by sprite
    elem.sprite_pak = pak;
    elem.sprite_index = sprite_idx;
    elem.sprite_frame = frame;
    return *this;
}

// === Element modification ===

dialog_builder& dialog_builder::tooltip(std::string_view text) {
    if (last_element_) {
        last_element_->tooltip = text;
    }
    return *this;
}

dialog_builder& dialog_builder::property(std::string_view key, std::string_view value) {
    if (last_element_) {
        last_element_->properties[std::string(key)] = std::string(value);
    }
    return *this;
}

// === Build ===

dialog_definition dialog_builder::build() {
    // Apply centering if requested
    if (def_.centered) {
        def_.bounds.x = (static_cast<int32_t>(screen_width) - def_.bounds.width) / 2;
        def_.bounds.y = (static_cast<int32_t>(screen_height) - def_.bounds.height) / 2;
    }

    return std::move(def_);
}

} // namespace hb
