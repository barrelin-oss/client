#include "graphics/renderer.hpp"
#include "graphics/color_utils.hpp"
#include "assets/sprite.hpp"
#include "core/config.hpp"
#include "platform/monitor.hpp"
#include <SFML/OpenGL.hpp>
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

    // Apply video settings from config
    const auto& video = config::instance().video();
    window_.setVerticalSyncEnabled(video.vsync);
    if (!video.vsync) {
        window_.setFramerateLimit(video.framerate_limit);
    }

    if (!window_.isOpen()) {
        spdlog::error("Failed to create window");
        return false;
    }

    // Position the window on the primary monitor
    if (!fullscreen) {
        if (video.remember_position && video.window_x >= 0 && video.window_y >= 0) {
            window_.setPosition({video.window_x, video.window_y});
            spdlog::info("Restored window position: {}, {}", video.window_x, video.window_y);
        } else if (auto mon = get_primary_monitor()) {
            int32_t center_x = mon->x + (mon->width - static_cast<int32_t>(width)) / 2;
            int32_t center_y = mon->y + (mon->height - static_cast<int32_t>(height)) / 2;
            window_.setPosition({center_x, center_y});
        }
    }

    // Hide the system cursor over the game window - the game draws its own software cursor.
    // SFML automatically restores the OS cursor when the mouse leaves the window.
    window_.setMouseCursorVisible(false);

    spdlog::info("Renderer initialized: {}x{} {}", width, height,
                 fullscreen ? "fullscreen" : "windowed");
    return true;
}

bool renderer::set_resolution(uint32_t width, uint32_t height, bool fullscreen) {
    // Capture the current window center before destroying it so we can
    // place the new window on the same monitor.
    std::optional<sf::Vector2i> prev_center;
    if (window_.isOpen()) {
        auto pos = window_.getPosition();
        prev_center = sf::Vector2i{
            pos.x + static_cast<int>(width_) / 2,
            pos.y + static_cast<int>(height_) / 2
        };
        window_.close();
    }

    // Update dimensions
    width_ = width;
    height_ = height;

    // Create new window with new resolution
    sf::VideoMode mode({width, height});

    if (fullscreen) {
        window_.create(mode, "Helbreath", sf::Style::None, sf::State::Fullscreen);
    } else {
        window_.create(mode, "Helbreath", sf::Style::Close);
    }

    // Restore video settings from config
    const auto& video = config::instance().video();
    window_.setVerticalSyncEnabled(video.vsync);
    if (!video.vsync) {
        window_.setFramerateLimit(video.framerate_limit);
    }

    if (!window_.isOpen()) {
        spdlog::error("Failed to recreate window at {}x{}", width, height);
        return false;
    }

    // Re-center the window around where the old window was, keeping it
    // on the same monitor. Use the same logic as initial window creation.
    if (!fullscreen) {
        if (video.remember_position && video.window_x >= 0 && video.window_y >= 0) {
            window_.setPosition({video.window_x, video.window_y});
        } else if (prev_center) {
            int32_t new_x = prev_center->x - static_cast<int32_t>(width) / 2;
            int32_t new_y = prev_center->y - static_cast<int32_t>(height) / 2;
            window_.setPosition({new_x, new_y});
        } else if (auto mon = get_primary_monitor()) {
            int32_t center_x = mon->x + (mon->width - static_cast<int32_t>(width)) / 2;
            int32_t center_y = mon->y + (mon->height - static_cast<int32_t>(height)) / 2;
            window_.setPosition({center_x, center_y});
        }
    }

    // Hide the system cursor (window was recreated)
    window_.setMouseCursorVisible(false);

    spdlog::info("Resolution changed to {}x{} {}", width, height,
                 fullscreen ? "fullscreen" : "windowed");
    return true;
}

void renderer::shutdown() {
    text_renderer_.shutdown();
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

        // Initialize text renderer with the loaded font
        text_renderer_.initialize(font_, window_);

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

void renderer::push_scissor(int32_t x, int32_t y, int32_t w, int32_t h) {
    // Flush SFML's render queue before changing OpenGL state
    window_.setActive(true);

    // OpenGL Y coordinate is flipped (0 at bottom)
    int32_t gl_y = static_cast<int32_t>(height_) - y - h;

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, gl_y, w, h);
}

void renderer::pop_scissor() {
    glDisable(GL_SCISSOR_TEST);
}

void renderer::set_zoom_view(float zoom_level, float center_x, float center_y) {
    sf::View view = window_.getDefaultView();
    view.setCenter({center_x, center_y});
    view.zoom(zoom_level);
    window_.setView(view);
}

void renderer::reset_to_default_view() {
    window_.setView(window_.getDefaultView());
}

} // namespace hb
