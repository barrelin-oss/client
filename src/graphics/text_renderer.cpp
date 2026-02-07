#include "graphics/text_renderer.hpp"
#include "graphics/text_shader_cache.hpp"
#include "graphics/color_utils.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hb {

text_renderer::text_renderer() = default;
text_renderer::~text_renderer() = default;

bool text_renderer::initialize(sf::Font& font, sf::RenderTarget& target)
{
    font_ = &font;
    target_ = &target;

    // Initialize shader cache
    shader_cache_ = std::make_unique<text_shader_cache>();
    shaders_available_ = shader_cache_->initialize();

    spdlog::info("Text renderer initialized (shaders: {})",
                 shaders_available_ ? "available" : "unavailable");
    return true;
}

void text_renderer::shutdown()
{
    shader_rt_.reset();
    shader_cache_.reset();
    font_ = nullptr;
    target_ = nullptr;
}

void text_renderer::set_target(sf::RenderTarget& target)
{
    target_ = &target;
}

void text_renderer::draw(std::string_view text, int32_t x, int32_t y,
                         const text_style& style, float time)
{
    if (!font_ || !target_)
    {
        return;
    }

    if (is_shader_effect(style.effect))
    {
        draw_shader_effect(text, x, y, style, time);
    }
    else
    {
        draw_cpu_effect(text, x, y, style, time);
    }
}

void text_renderer::draw(std::string_view text, int32_t x, int32_t y,
                         sf::Color color, uint32_t size)
{
    text_style style;
    style.color = color;
    style.size = size;
    style.effect = text_effect::none;
    draw(text, x, y, style, 0.0f);
}

float text_renderer::measure_width(std::string_view text, uint32_t size) const
{
    if (!font_)
    {
        return 0.0f;
    }

    float width = 0.0f;
    for (char c : text)
    {
        width += glyph_advance(c, size);
    }
    return width;
}

// ---------------------------------------------------------------------------
// CPU effect dispatch
// ---------------------------------------------------------------------------

void text_renderer::draw_cpu_effect(std::string_view text, int32_t x, int32_t y,
                                    const text_style& style, float time)
{
    // Calculate effective alpha (255 unless a specific effect modifies it)
    uint8_t alpha = style.color.a;

    switch (style.effect)
    {
        case text_effect::none:
            draw_none(text, x, y, style, alpha);
            break;
        case text_effect::rainbow:
            draw_rainbow(text, x, y, style, time, alpha);
            break;
        case text_effect::special:
            draw_special(text, x, y, style, time, alpha);
            break;
        case text_effect::terror:
            draw_terror(text, x, y, style, time, alpha);
            break;
        case text_effect::pulsing:
            draw_pulsing(text, x, y, style, time, alpha);
            break;
        case text_effect::wave:
            draw_wave(text, x, y, style, time, alpha);
            break;
        case text_effect::glitch:
            draw_glitch(text, x, y, style, time, alpha);
            break;
        case text_effect::typewriter:
            draw_typewriter(text, x, y, style, time, alpha);
            break;
        default:
            // Unknown CPU effect - render as plain text
            draw_none(text, x, y, style, alpha);
            break;
    }
}

// ---------------------------------------------------------------------------
// GPU shader effect pipeline
// ---------------------------------------------------------------------------

void text_renderer::draw_shader_effect(std::string_view text, int32_t x, int32_t y,
                                       const text_style& style, float time)
{
    // Fallback: if shaders not available, render as plain outlined text
    sf::Shader* shader = shaders_available_ ? shader_cache_->get(style.effect) : nullptr;
    if (!shader)
    {
        draw_none(text, x, y, style, style.color.a);
        return;
    }

    // 1. Measure text bounds
    sf::Text sf_text(*font_, std::string(text), style.size);
    sf_text.setFillColor(style.color);
    if (style.outline_thickness > 0.0f)
    {
        sf_text.setOutlineColor(style.outline_color);
        sf_text.setOutlineThickness(style.outline_thickness);
    }

    auto bounds = sf_text.getLocalBounds();
    constexpr float padding = 8.0f;
    auto needed_w = static_cast<uint32_t>(bounds.size.x + bounds.position.x + padding * 2);
    auto needed_h = static_cast<uint32_t>(bounds.size.y + bounds.position.y + padding * 2);

    // Minimum size
    if (needed_w < 16) needed_w = 16;
    if (needed_h < 16) needed_h = 16;

    // 2. Ensure render texture is big enough (lazily resize)
    if (!shader_rt_ || shader_rt_width_ < needed_w || shader_rt_height_ < needed_h)
    {
        uint32_t new_w = std::max(needed_w, shader_rt_width_);
        uint32_t new_h = std::max(needed_h, shader_rt_height_);
        // Round up to power of 2 for efficiency
        new_w = std::max(new_w, 64u);
        new_h = std::max(new_h, 64u);

        shader_rt_ = std::make_unique<sf::RenderTexture>(sf::Vector2u{new_w, new_h});
        shader_rt_width_ = new_w;
        shader_rt_height_ = new_h;
    }

    // 3. Render text into render texture
    shader_rt_->clear(sf::Color::Transparent);
    sf_text.setPosition({padding, padding});
    shader_rt_->draw(sf_text);
    shader_rt_->display();

    // 4. Set shader uniforms (only common ones that every shader uses)
    shader->setUniform("u_texture", sf::Shader::CurrentTexture);
    shader->setUniform("u_time", time);

    // Per-effect uniforms (u_resolution only set for shaders that use it)
    sf::Glsl::Vec2 resolution(
        static_cast<float>(shader_rt_width_),
        static_cast<float>(shader_rt_height_));

    switch (style.effect)
    {
        case text_effect::dissolve:
            shader->setUniform("u_burn_color", sf::Glsl::Vec4(1.0f, 0.4f, 0.1f, 1.0f));
            break;
        case text_effect::chromatic:
            shader->setUniform("u_resolution", resolution);
            shader->setUniform("u_offset", 3.0f);
            break;
        case text_effect::glow:
            shader->setUniform("u_resolution", resolution);
            shader->setUniform("u_glow_radius", 2.0f);
            shader->setUniform("u_glow_color", sf::Glsl::Vec4(
                style.color.r / 255.0f,
                style.color.g / 255.0f,
                style.color.b / 255.0f,
                0.6f));
            break;
        case text_effect::distortion:
            shader->setUniform("u_resolution", resolution);
            shader->setUniform("u_amplitude", 2.0f);
            shader->setUniform("u_frequency", 10.0f);
            break;
        case text_effect::scanlines:
            shader->setUniform("u_resolution", resolution);
            shader->setUniform("u_line_spacing", 3.0f);
            shader->setUniform("u_line_alpha", 0.3f);
            break;
        default:
            break;
    }

    // 5. Draw the textured sprite with shader applied
    sf::Sprite sprite(shader_rt_->getTexture());
    sprite.setPosition({static_cast<float>(x) - padding,
                        static_cast<float>(y) - padding});

    target_->draw(sprite, sf::RenderStates(shader));
}

// ---------------------------------------------------------------------------
// Individual CPU effects
// ---------------------------------------------------------------------------

void text_renderer::draw_none(std::string_view text, int32_t x, int32_t y,
                              const text_style& style, uint8_t alpha)
{
    sf::Text sf_text(*font_, std::string(text), style.size);
    sf::Color color = style.color;
    color.a = alpha;
    sf_text.setFillColor(color);

    if (style.outline_thickness > 0.0f)
    {
        sf::Color outline = style.outline_color;
        outline.a = alpha;
        sf_text.setOutlineColor(outline);
        sf_text.setOutlineThickness(style.outline_thickness);
    }

    sf_text.setPosition({static_cast<float>(x), static_cast<float>(y)});
    target_->draw(sf_text);
}

void text_renderer::draw_rainbow(std::string_view text, int32_t x, int32_t y,
                                 const text_style& style, float time, uint8_t alpha)
{
    sf::Color color = hue_to_rgb(time);
    color.a = alpha;

    sf::Text sf_text(*font_, std::string(text), style.size);
    sf_text.setFillColor(color);

    if (style.outline_thickness > 0.0f)
    {
        sf::Color outline = style.outline_color;
        outline.a = alpha;
        sf_text.setOutlineColor(outline);
        sf_text.setOutlineThickness(style.outline_thickness);
    }

    sf_text.setPosition({static_cast<float>(x), static_cast<float>(y)});
    target_->draw(sf_text);
}

void text_renderer::draw_special(std::string_view text, int32_t x, int32_t y,
                                 const text_style& style, float time, uint8_t alpha)
{
    float char_x = static_cast<float>(x);
    constexpr float hue_step = 0.08f;

    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == ' ')
        {
            char_x += glyph_advance(' ', style.size);
            continue;
        }

        sf::Color color = hue_to_rgb(time + i * hue_step);
        color.a = alpha;

        sf::Text sf_text(*font_, std::string(1, text[i]), style.size);
        sf_text.setFillColor(color);
        if (style.outline_thickness > 0.0f)
        {
            sf::Color outline = style.outline_color;
            outline.a = alpha;
            sf_text.setOutlineColor(outline);
            sf_text.setOutlineThickness(style.outline_thickness);
        }
        sf_text.setPosition({char_x, static_cast<float>(y)});
        target_->draw(sf_text);

        char_x += glyph_advance(text[i], style.size);
    }
}

void text_renderer::draw_terror(std::string_view text, int32_t x, int32_t y,
                                const text_style& style, float time, uint8_t alpha)
{
    sf::Color color = style.color;
    color.a = alpha;
    float char_x = static_cast<float>(x);
    constexpr float bounce_amplitude = 2.0f;
    constexpr float min_freq = 15.0f;
    constexpr float max_freq = 25.0f;

    auto pseudo_random = [](size_t index, char c, uint32_t seed) -> float {
        uint32_t hash = static_cast<uint32_t>(index) * 2654435761u;
        hash ^= static_cast<uint32_t>(c) * 2246822519u;
        hash ^= seed * 3266489917u;
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = (hash >> 16) ^ hash;
        return static_cast<float>(hash & 0xFFFF) / 65535.0f;
    };

    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == ' ')
        {
            char_x += glyph_advance(' ', style.size);
            continue;
        }

        float rand_freq = min_freq + pseudo_random(i, text[i], 1) * (max_freq - min_freq);
        float rand_phase = pseudo_random(i, text[i], 2) * 6.283185f;
        float phase = time * rand_freq + rand_phase;
        float y_offset = std::sin(phase) * bounce_amplitude;

        sf::Text sf_text(*font_, std::string(1, text[i]), style.size);
        sf_text.setFillColor(color);
        if (style.outline_thickness > 0.0f)
        {
            sf::Color outline = style.outline_color;
            outline.a = alpha;
            sf_text.setOutlineColor(outline);
            sf_text.setOutlineThickness(style.outline_thickness);
        }
        sf_text.setPosition({char_x, static_cast<float>(y) + y_offset});
        target_->draw(sf_text);

        char_x += glyph_advance(text[i], style.size);
    }
}

void text_renderer::draw_pulsing(std::string_view text, int32_t x, int32_t y,
                                 const text_style& style, float time, uint8_t alpha)
{
    constexpr float pulse_freq = 3.0f;
    float pulse = 0.5f * (1.0f + std::sin(time * pulse_freq * 6.283185f));
    float alpha_factor = 0.4f + 0.6f * pulse;
    auto pulsed_alpha = static_cast<uint8_t>(alpha * alpha_factor);

    // Delegate to draw_none with modified alpha
    text_style pulsed_style = style;
    pulsed_style.color.a = pulsed_alpha;
    draw_none(text, x, y, pulsed_style, pulsed_alpha);
}

void text_renderer::draw_wave(std::string_view text, int32_t x, int32_t y,
                              const text_style& style, float time, uint8_t alpha)
{
    sf::Color color = style.color;
    color.a = alpha;
    float char_x = static_cast<float>(x);
    constexpr float wave_amplitude = 3.0f;
    constexpr float wave_freq = 3.0f;
    constexpr float phase_step = 0.5f;

    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == ' ')
        {
            char_x += glyph_advance(' ', style.size);
            continue;
        }

        float phase = time * wave_freq * 6.283185f + i * phase_step;
        float y_offset = std::sin(phase) * wave_amplitude;

        sf::Text sf_text(*font_, std::string(1, text[i]), style.size);
        sf_text.setFillColor(color);
        if (style.outline_thickness > 0.0f)
        {
            sf::Color outline = style.outline_color;
            outline.a = alpha;
            sf_text.setOutlineColor(outline);
            sf_text.setOutlineThickness(style.outline_thickness);
        }
        sf_text.setPosition({char_x, static_cast<float>(y) + y_offset});
        target_->draw(sf_text);

        char_x += glyph_advance(text[i], style.size);
    }
}

void text_renderer::draw_glitch(std::string_view text, int32_t x, int32_t y,
                                const text_style& style, float time, uint8_t alpha)
{
    sf::Color color = style.color;
    color.a = alpha;
    float char_x = static_cast<float>(x);

    static constexpr std::string_view glitch_chars = "!@#$%&*?/\\|<>~^";
    auto time_slot = static_cast<uint32_t>(time * 12.0f);

    auto glitch_hash = [](size_t index, uint32_t slot) -> uint32_t {
        uint32_t hash = static_cast<uint32_t>(index) * 2654435761u;
        hash ^= slot * 2246822519u;
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = (hash >> 16) ^ hash;
        return hash;
    };

    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == ' ')
        {
            char_x += glyph_advance(' ', style.size);
            continue;
        }

        uint32_t hash = glitch_hash(i, time_slot);
        bool is_glitched = (hash % 100) < 15;

        char display_char = text[i];
        sf::Color char_color = color;

        if (is_glitched)
        {
            display_char = glitch_chars[hash % glitch_chars.size()];
            char_color = sf::Color(
                static_cast<uint8_t>(std::min(255, static_cast<int>(color.r) + 30)),
                static_cast<uint8_t>(std::max(0, static_cast<int>(color.g) - 50)),
                static_cast<uint8_t>(std::max(0, static_cast<int>(color.b) - 50)),
                alpha
            );
        }

        sf::Text sf_text(*font_, std::string(1, display_char), style.size);
        sf_text.setFillColor(char_color);
        if (style.outline_thickness > 0.0f)
        {
            sf::Color outline = style.outline_color;
            outline.a = alpha;
            sf_text.setOutlineColor(outline);
            sf_text.setOutlineThickness(style.outline_thickness);
        }
        sf_text.setPosition({char_x, static_cast<float>(y)});
        target_->draw(sf_text);

        // Advance by original glyph width so text doesn't jitter
        char_x += glyph_advance(text[i], style.size);
    }
}

void text_renderer::draw_typewriter(std::string_view text, int32_t x, int32_t y,
                                    const text_style& style, float time, uint8_t alpha)
{
    constexpr float chars_per_second = 20.0f;
    auto visible_count = static_cast<size_t>(time * chars_per_second);
    if (visible_count > text.size())
    {
        visible_count = text.size();
    }

    if (visible_count == 0)
    {
        return;
    }

    std::string_view visible = text.substr(0, visible_count);
    draw_none(visible, x, y, style, alpha);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void text_renderer::draw_char(char c, float x, float y, sf::Color color,
                              const text_style& style, uint8_t alpha)
{
    sf::Text sf_text(*font_, std::string(1, c), style.size);
    color.a = alpha;
    sf_text.setFillColor(color);
    if (style.outline_thickness > 0.0f)
    {
        sf::Color outline = style.outline_color;
        outline.a = alpha;
        sf_text.setOutlineColor(outline);
        sf_text.setOutlineThickness(style.outline_thickness);
    }
    sf_text.setPosition({x, y});
    target_->draw(sf_text);
}

float text_renderer::glyph_advance(char c, uint32_t size) const
{
    if (!font_)
    {
        return 0.0f;
    }
    auto glyph = font_->getGlyph(static_cast<uint32_t>(c), size, false);
    return glyph.advance;
}

} // namespace hb
