#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include <string_view>

namespace hb {

class sprite;

class renderer {
public:
    bool initialize(uint32_t width, uint32_t height, bool fullscreen);
    void shutdown();

    void begin_frame();
    void end_frame();

    // Sprite drawing (with color key transparency)
    void draw_sprite(const sprite& spr, int32_t x, int32_t y, uint32_t frame = 0);
    void draw_sprite_alpha(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha);

    // Sprite drawing without color key (for backgrounds, etc.)
    void draw_sprite_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame = 0);
    void draw_sprite_alpha_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha);

    // Raw texture drawing
    void draw_texture(const sf::Texture& texture, int32_t x, int32_t y);
    void draw_texture(const sf::Texture& texture, int32_t x, int32_t y, const sf::IntRect& rect);

    // Text rendering
    bool load_font(std::string_view path);
    void draw_text(std::string_view text, int32_t x, int32_t y, sf::Color color = sf::Color::White);
    void draw_text(std::string_view text, int32_t x, int32_t y, sf::Color color, uint32_t size);

    // Primitives
    void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, sf::Color color, bool filled = true);
    void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, sf::Color color);

    // Accessors
    sf::RenderWindow& window() { return window_; }
    const sf::RenderWindow& window() const { return window_; }
    bool is_open() const { return window_.isOpen(); }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

private:
    sf::RenderWindow window_;
    sf::Font font_;
    bool font_loaded_ = false;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

} // namespace hb
