#pragma once

#include "graphics/text_style.hpp"
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <string_view>

namespace hb {

class text_shader_cache;

// Unified text rendering system supporting CPU effects (per-character)
// and GPU effects (render-to-texture + fragment shader).
// Owned by renderer, accessed via renderer::text().
class text_renderer
{
public:
    text_renderer();
    ~text_renderer();

    // Initialize with font and render target references
    bool initialize(sf::Font& font, sf::RenderTarget& target);
    void shutdown();

    // Switch render target (called by renderer when switching between window and scene_rt_)
    void set_target(sf::RenderTarget& target);

    // Primary: draw styled text at screen position
    // time = elapsed seconds for animation (0 for static)
    void draw(std::string_view text, int32_t x, int32_t y,
              const text_style& style, float time = 0.0f);

    // Convenience: plain text (no effect, no outline)
    void draw(std::string_view text, int32_t x, int32_t y,
              sf::Color color, uint32_t size = 14);

    // Measurement (for centering text)
    float measure_width(std::string_view text, uint32_t size) const;

private:
    // CPU effect dispatch
    void draw_cpu_effect(std::string_view text, int32_t x, int32_t y,
                         const text_style& style, float time);

    // GPU shader effect pipeline (render-to-texture + shader)
    void draw_shader_effect(std::string_view text, int32_t x, int32_t y,
                            const text_style& style, float time);

    // Individual CPU effects
    void draw_none(std::string_view text, int32_t x, int32_t y,
                   const text_style& style, uint8_t alpha);
    void draw_rainbow(std::string_view text, int32_t x, int32_t y,
                      const text_style& style, float time, uint8_t alpha);
    void draw_special(std::string_view text, int32_t x, int32_t y,
                      const text_style& style, float time, uint8_t alpha);
    void draw_terror(std::string_view text, int32_t x, int32_t y,
                     const text_style& style, float time, uint8_t alpha);
    void draw_pulsing(std::string_view text, int32_t x, int32_t y,
                      const text_style& style, float time, uint8_t alpha);
    void draw_wave(std::string_view text, int32_t x, int32_t y,
                   const text_style& style, float time, uint8_t alpha);
    void draw_glitch(std::string_view text, int32_t x, int32_t y,
                     const text_style& style, float time, uint8_t alpha);
    void draw_typewriter(std::string_view text, int32_t x, int32_t y,
                         const text_style& style, float time, uint8_t alpha);

    // Helper: draw a single character with optional outline
    void draw_char(char c, float x, float y, sf::Color color,
                   const text_style& style, uint8_t alpha);

    // Helper: get glyph advance for a character
    float glyph_advance(char c, uint32_t size) const;

    sf::Font* font_ = nullptr;
    sf::RenderTarget* target_ = nullptr;

    // Render texture for shader effects (lazily created)
    std::unique_ptr<sf::RenderTexture> shader_rt_;
    uint32_t shader_rt_width_ = 0;
    uint32_t shader_rt_height_ = 0;

    // Shader cache
    std::unique_ptr<text_shader_cache> shader_cache_;
    bool shaders_available_ = false;
};

} // namespace hb
