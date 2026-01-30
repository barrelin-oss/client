#include "graphics/renderer.hpp"
#include "assets/sprite.hpp"
#include <spdlog/spdlog.h>

namespace hb {

bool renderer::initialize(uint32_t width, uint32_t height, bool fullscreen) {
    width_ = width;
    height_ = height;

    // SFML 3.0 uses sf::VideoMode with Vector2u
    sf::VideoMode mode({width, height});

    // SFML 3.0 uses sf::State for fullscreen
    if (fullscreen) {
        window_.create(mode, "Helbreath", sf::Style::None, sf::State::Fullscreen);
    } else {
        window_.create(mode, "Helbreath", sf::Style::Close);
    }

    window_.setFramerateLimit(60);

    if (!window_.isOpen()) {
        spdlog::error("Failed to create window");
        return false;
    }

    spdlog::info("Renderer initialized: {}x{} {}", width, height,
                 fullscreen ? "fullscreen" : "windowed");
    return true;
}

void renderer::shutdown() {
    if (window_.isOpen()) {
        window_.close();
    }
    spdlog::info("Renderer shutdown");
}

void renderer::begin_frame() {
    window_.clear(sf::Color::Black);
}

void renderer::end_frame() {
    window_.display();
}

void renderer::draw_sprite(const sprite& spr, int32_t x, int32_t y, uint32_t frame) {
    spr.draw(window_, x, y, frame);
}

void renderer::draw_sprite_alpha(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha) {
    spr.draw_alpha(window_, x, y, frame, alpha);
}

void renderer::draw_sprite_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame) {
    spr.draw_no_color_key(window_, x, y, frame);
}

void renderer::draw_sprite_alpha_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha) {
    spr.draw_alpha_no_color_key(window_, x, y, frame, alpha);
}

void renderer::draw_texture(const sf::Texture& texture, int32_t x, int32_t y) {
    sf::Sprite spr(texture);
    spr.setPosition({static_cast<float>(x), static_cast<float>(y)});
    window_.draw(spr);
}

void renderer::draw_texture(const sf::Texture& texture, int32_t x, int32_t y, const sf::IntRect& rect) {
    sf::Sprite spr(texture, rect);
    spr.setPosition({static_cast<float>(x), static_cast<float>(y)});
    window_.draw(spr);
}

bool renderer::load_font(std::string_view path) {
    if (font_.openFromFile(std::string(path))) {
        font_loaded_ = true;
        spdlog::info("Font loaded: {}", path);
        return true;
    }
    spdlog::error("Failed to load font: {}", path);
    return false;
}

void renderer::draw_text(std::string_view text, int32_t x, int32_t y, sf::Color color) {
    draw_text(text, x, y, color, 14);
}

void renderer::draw_text(std::string_view text, int32_t x, int32_t y, sf::Color color, uint32_t size) {
    if (!font_loaded_) {
        return;
    }

    sf::Text sf_text(font_, std::string(text), size);
    sf_text.setFillColor(color);
    sf_text.setPosition({static_cast<float>(x), static_cast<float>(y)});
    window_.draw(sf_text);
}

void renderer::draw_text_outlined(std::string_view text, int32_t x, int32_t y,
                                  sf::Color color, sf::Color outline_color,
                                  uint32_t size, float outline_thickness) {
    if (!font_loaded_) {
        return;
    }

    sf::Text sf_text(font_, std::string(text), size);
    sf_text.setFillColor(color);
    sf_text.setOutlineColor(outline_color);
    sf_text.setOutlineThickness(outline_thickness);
    sf_text.setPosition({static_cast<float>(x), static_cast<float>(y)});
    window_.draw(sf_text);
}

void renderer::draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, sf::Color color, bool filled) {
    sf::RectangleShape rect({static_cast<float>(w), static_cast<float>(h)});
    rect.setPosition({static_cast<float>(x), static_cast<float>(y)});

    if (filled) {
        rect.setFillColor(color);
        rect.setOutlineThickness(0);
    } else {
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(color);
        rect.setOutlineThickness(1);
    }

    window_.draw(rect);
}

void renderer::draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, sf::Color color) {
    std::array<sf::Vertex, 2> line = {
        sf::Vertex{{static_cast<float>(x1), static_cast<float>(y1)}, color},
        sf::Vertex{{static_cast<float>(x2), static_cast<float>(y2)}, color}
    };
    window_.draw(line.data(), 2, sf::PrimitiveType::Lines);
}

} // namespace hb
