#include "graphics/renderer.hpp"
#include "graphics/color_utils.hpp"
#include "assets/sprite.hpp"
#include "core/config.hpp"
#include "platform/monitor.hpp"
#include <SFML/OpenGL.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
// X11 defines None as a macro (0L) which conflicts with sf::Style::None
#undef None
#endif

namespace hb {

// Static shader members
sf::Shader renderer::additive_shader_;
bool renderer::additive_shader_loaded_ = false;
bool renderer::additive_shader_init_attempted_ = false;

sf::Shader renderer::tint_shader_;
bool renderer::tint_shader_loaded_ = false;
bool renderer::tint_shader_init_attempted_ = false;

bool renderer::initialize(uint32_t width, uint32_t height, bool fullscreen,
                          bool borderless, int32_t monitor_x, int32_t monitor_y) {
    width_ = width;
    height_ = height;
    borderless_ = borderless;

    // SFML 3.0 uses sf::VideoMode with Vector2u
    sf::VideoMode mode({width, height});

    if (fullscreen) {
        // True exclusive fullscreen
        window_.create(mode, "Helbreath", sf::Style::None, sf::State::Fullscreen);
    } else if (borderless) {
        // Borderless windowed: no titlebar, no display mode change
        window_.create(mode, "Helbreath", sf::Style::None, sf::State::Windowed);
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

    // Initialize active_target_ to the window
    active_target_ = &window_;

    // Position the window
    if (borderless) {
        // Place borderless window at the target monitor's origin
        window_.setPosition({monitor_x, monitor_y});
        // Set topmost so the window renders above the OS taskbar
        set_topmost(true);
    } else if (!fullscreen) {
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

    const char* mode_str = fullscreen ? "fullscreen" : (borderless ? "borderless" : "windowed");
    spdlog::info("Renderer initialized: {}x{} {}", width, height, mode_str);
    return true;
}

bool renderer::set_resolution(uint32_t width, uint32_t height, bool fullscreen,
                              bool borderless, int32_t monitor_x, int32_t monitor_y) {
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
    } else if (borderless) {
        window_.create(mode, "Helbreath", sf::Style::None, sf::State::Windowed);
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

    // Update borderless state
    borderless_ = borderless;

    // Reset active_target_ to the window
    active_target_ = &window_;

    // Recreate scene render texture if in scaled mode
    if (view_mode_ == view_mode::scaled) {
        create_scene_rt();
    }

    // Position the window
    if (borderless) {
        window_.setPosition({monitor_x, monitor_y});
        set_topmost(true);
    } else if (!fullscreen) {
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

    const char* mode_str = fullscreen ? "fullscreen" : (borderless ? "borderless" : "windowed");
    spdlog::info("Resolution changed to {}x{} {}", width, height, mode_str);
    return true;
}

void renderer::shutdown() {
    destroy_scene_rt();
    text_renderer_.shutdown();
    if (window_.isOpen()) {
        window_.close();
    }
    spdlog::info("Renderer shutdown");
}

void renderer::begin_frame() {
    window_.clear(sf::Color::Black);
    active_target_ = &window_;
    rendering_scene_ = false;
    draw_call_count_ = 0;
}

void renderer::end_frame() {
    window_.display();
}

// ---------------------------------------------------------------------------
// View mode system
// ---------------------------------------------------------------------------

void renderer::set_view_mode(view_mode mode) {
    if (view_mode_ == mode) return;

    view_mode prev = view_mode_;
    view_mode_ = mode;

    if (mode == view_mode::scaled) {
        create_scene_rt();
    } else if (prev == view_mode::scaled) {
        destroy_scene_rt();
    }

    spdlog::info("View mode changed to {}", static_cast<int>(mode));
}

void renderer::set_internal_resolution(uint32_t w, uint32_t h) {
    if (internal_width_ == w && internal_height_ == h) return;

    internal_width_ = w;
    internal_height_ = h;

    // Recreate scene RT if it exists
    if (view_mode_ == view_mode::scaled) {
        create_scene_rt();
    }

    spdlog::info("Internal resolution set to {}x{}", w, h);
}

void renderer::begin_scene() {
    if (view_mode_ == view_mode::scaled && scene_rt_) {
        active_target_ = scene_rt_.get();
        scene_rt_->clear(sf::Color::Black);
        rendering_scene_ = true;
        text_renderer_.set_target(*active_target_);
    }
}

void renderer::end_scene() {
    if (view_mode_ == view_mode::scaled && scene_rt_) {
        // Finalize the render texture
        scene_rt_->display();

        // Switch back to window
        active_target_ = &window_;
        rendering_scene_ = false;
        text_renderer_.set_target(*active_target_);

        // Composite scene_rt_ onto window
        const auto& tex = scene_rt_->getTexture();
        auto tex_smooth = scale_filter_ == scale_filter::bilinear;
        // SFML 3 textures are const from RenderTexture, use setSmooth on rt before display
        // We set smooth on the render texture itself
        scene_rt_->setSmooth(tex_smooth);

        sf::Sprite scene_sprite(tex);

        float display_w = static_cast<float>(width_);
        float display_h = static_cast<float>(height_);
        float internal_w = static_cast<float>(internal_width_);
        float internal_h = static_cast<float>(internal_height_);

        if (aspect_mode_ == aspect_mode::letterbox) {
            float scale = std::min(display_w / internal_w, display_h / internal_h);
            float scaled_w = internal_w * scale;
            float scaled_h = internal_h * scale;
            float offset_x = (display_w - scaled_w) / 2.0f;
            float offset_y = (display_h - scaled_h) / 2.0f;

            scene_sprite.setPosition({offset_x, offset_y});
            scene_sprite.setScale({scale, scale});
        } else {
            // Stretch mode
            float scale_x = display_w / internal_w;
            float scale_y = display_h / internal_h;
            scene_sprite.setScale({scale_x, scale_y});
        }

        window_.draw(scene_sprite);
    }
    else if (view_mode_ == view_mode::extended) {
        auto fair = fair_bounds();
        auto dw = static_cast<int32_t>(width_);
        auto dh = static_cast<int32_t>(height_);

        switch (fog_style_) {
            case fog_style::solid:    draw_fog_solid(fair, dw, dh); break;
            case fog_style::tile_fog: draw_fog_tile(fair, dw, dh); break;
            case fog_style::vignette: draw_fog_vignette(fair, dw, dh); break;
            case fog_style::gradient: draw_fog_gradient(fair, dw, dh); break;
        }

        if (targeting_boundary_visible_) {
            draw_targeting_boundary(fair);
        }
    }
    // Special mode: no-op
}

void renderer::begin_ui() {
    active_target_ = &window_;
    rendering_scene_ = false;
    text_renderer_.set_target(window_);
    window_.setView(window_.getDefaultView());
}

uint32_t renderer::scene_width() const {
    if (view_mode_ == view_mode::scaled) {
        return internal_width_;
    }
    return width_;
}

uint32_t renderer::scene_height() const {
    if (view_mode_ == view_mode::scaled) {
        return internal_height_;
    }
    return height_;
}

uint32_t renderer::interaction_width() const {
    if (view_mode_ == view_mode::special) return width_;
    return internal_width_;
}

uint32_t renderer::interaction_height() const {
    if (view_mode_ == view_mode::special) return height_;
    return internal_height_;
}

std::pair<int32_t, int32_t> renderer::display_to_scene(int32_t x, int32_t y) const {
    if (view_mode_ == view_mode::scaled) {
        float display_w = static_cast<float>(width_);
        float display_h = static_cast<float>(height_);
        float internal_w = static_cast<float>(internal_width_);
        float internal_h = static_cast<float>(internal_height_);

        if (aspect_mode_ == aspect_mode::letterbox) {
            float scale = std::min(display_w / internal_w, display_h / internal_h);
            float offset_x = (display_w - internal_w * scale) / 2.0f;
            float offset_y = (display_h - internal_h * scale) / 2.0f;

            auto scene_x = static_cast<int32_t>((static_cast<float>(x) - offset_x) / scale);
            auto scene_y = static_cast<int32_t>((static_cast<float>(y) - offset_y) / scale);
            return {scene_x, scene_y};
        } else {
            float scale_x = display_w / internal_w;
            float scale_y = display_h / internal_h;
            auto scene_x = static_cast<int32_t>(static_cast<float>(x) / scale_x);
            auto scene_y = static_cast<int32_t>(static_cast<float>(y) / scale_y);
            return {scene_x, scene_y};
        }
    }
    // Extended and special: 1:1
    return {x, y};
}

std::pair<int32_t, int32_t> renderer::scene_to_display(int32_t x, int32_t y) const {
    if (view_mode_ == view_mode::scaled) {
        float display_w = static_cast<float>(width_);
        float display_h = static_cast<float>(height_);
        float internal_w = static_cast<float>(internal_width_);
        float internal_h = static_cast<float>(internal_height_);

        if (aspect_mode_ == aspect_mode::letterbox) {
            float scale = std::min(display_w / internal_w, display_h / internal_h);
            float offset_x = (display_w - internal_w * scale) / 2.0f;
            float offset_y = (display_h - internal_h * scale) / 2.0f;

            auto disp_x = static_cast<int32_t>(static_cast<float>(x) * scale + offset_x);
            auto disp_y = static_cast<int32_t>(static_cast<float>(y) * scale + offset_y);
            return {disp_x, disp_y};
        } else {
            float scale_x = display_w / internal_w;
            float scale_y = display_h / internal_h;
            auto disp_x = static_cast<int32_t>(static_cast<float>(x) * scale_x);
            auto disp_y = static_cast<int32_t>(static_cast<float>(y) * scale_y);
            return {disp_x, disp_y};
        }
    }
    return {x, y};
}

sf::IntRect renderer::fair_bounds() const {
    if (view_mode_ == view_mode::extended) {
        int32_t fair_x = (static_cast<int32_t>(width_) - static_cast<int32_t>(internal_width_)) / 2;
        int32_t fair_y = (static_cast<int32_t>(height_) - static_cast<int32_t>(internal_height_)) / 2;
        return sf::IntRect(
            {fair_x, fair_y},
            {static_cast<int32_t>(internal_width_), static_cast<int32_t>(internal_height_)}
        );
    }
    // For other modes, the entire screen is "fair"
    return sf::IntRect({0, 0}, {static_cast<int32_t>(width_), static_cast<int32_t>(height_)});
}

bool renderer::is_in_fair_zone(int32_t display_x, int32_t display_y) const {
    if (view_mode_ != view_mode::extended) return true;
    auto fair = fair_bounds();
    return display_x >= fair.position.x && display_x < fair.position.x + fair.size.x &&
           display_y >= fair.position.y && display_y < fair.position.y + fair.size.y;
}

void renderer::create_scene_rt() {
    scene_rt_ = std::make_unique<sf::RenderTexture>(sf::Vector2u{internal_width_, internal_height_});
    spdlog::info("Created scene render texture: {}x{}", internal_width_, internal_height_);
}

void renderer::destroy_scene_rt() {
    scene_rt_.reset();
}

// ---------------------------------------------------------------------------
// Fog overlay styles
// ---------------------------------------------------------------------------

void renderer::set_fog_style(fog_style style) {
    fog_style_ = style;
    spdlog::info("Fog style changed to {}", static_cast<int>(style));
}

void renderer::set_fog_camera_info(int32_t camera_x, int32_t camera_y, float zoom) {
    fog_camera_x_ = camera_x;
    fog_camera_y_ = camera_y;
    fog_zoom_ = zoom;
}

void renderer::draw_fog_solid(const sf::IntRect& fair, int32_t dw, int32_t dh) {
    sf::Color fog(0, 0, 0, 200);
    if (fair.position.y > 0)
        draw_rect(0, 0, dw, fair.position.y, fog);
    int32_t bot = fair.position.y + fair.size.y;
    if (bot < dh)
        draw_rect(0, bot, dw, dh - bot, fog);
    if (fair.position.x > 0)
        draw_rect(0, fair.position.y, fair.position.x, fair.size.y, fog);
    int32_t right = fair.position.x + fair.size.x;
    if (right < dw)
        draw_rect(right, fair.position.y, dw - right, fair.size.y, fog);
}

void renderer::draw_fog_tile(const sf::IntRect& fair, int32_t dw, int32_t dh) {
    constexpr float tile_size = 32.0f;
    float zoom = std::max(fog_zoom_, 0.1f);
    float step = tile_size / zoom;

    // Calculate tile grid phase on screen from camera position
    float cx = static_cast<float>(dw) / 2.0f;
    float cy = static_cast<float>(dh) / 2.0f;
    float origin_x = cx + (static_cast<float>(-fog_camera_x_) - cx) / zoom;
    float origin_y = cy + (static_cast<float>(-fog_camera_y_) - cy) / zoom;
    float phase_x = std::fmod(origin_x, step);
    if (phase_x < 0) phase_x += step;
    float phase_y = std::fmod(origin_y, step);
    if (phase_y < 0) phase_y += step;

    // Snap fair zone inward to tile boundaries
    float fl = static_cast<float>(fair.position.x);
    float ft = static_cast<float>(fair.position.y);
    float fr = static_cast<float>(fair.position.x + fair.size.x);
    float fb = static_cast<float>(fair.position.y + fair.size.y);

    auto snap_up = [](float val, float ph, float s) {
        return std::ceil((val - ph) / s) * s + ph;
    };
    auto snap_down = [](float val, float ph, float s) {
        return std::floor((val - ph) / s) * s + ph;
    };

    int32_t sl = static_cast<int32_t>(snap_up(fl, phase_x, step));
    int32_t st = static_cast<int32_t>(snap_up(ft, phase_y, step));
    int32_t sr = static_cast<int32_t>(snap_down(fr, phase_x, step));
    int32_t sb = static_cast<int32_t>(snap_down(fb, phase_y, step));

    sf::Color fog(0, 0, 0, 200);
    if (st > 0)  draw_rect(0, 0, dw, st, fog);
    if (sb < dh) draw_rect(0, sb, dw, dh - sb, fog);
    if (sl > 0)  draw_rect(0, st, sl, sb - st, fog);
    if (sr < dw) draw_rect(sr, st, dw - sr, sb - st, fog);
}

void renderer::draw_fog_vignette(const sf::IntRect& fair, int32_t dw, int32_t dh) {
    constexpr int segments = 64;
    constexpr uint8_t max_alpha = 220;
    constexpr float edge_width = 40.0f;  // Sharp transition zone width
    constexpr float pi2 = 6.2831853f;

    float cx = static_cast<float>(dw) / 2.0f;
    float cy = static_cast<float>(dh) / 2.0f;

    // Inner ellipse matches fair zone
    float rx_inner = static_cast<float>(fair.size.x) / 2.0f;
    float ry_inner = static_cast<float>(fair.size.y) / 2.0f;

    // Mid ellipse: sharp transition zone just outside fair zone
    float rx_mid = rx_inner + edge_width;
    float ry_mid = ry_inner + edge_width;

    // Outer radius covers screen corners
    float outer_r = std::sqrt(cx * cx + cy * cy) + 50.0f;

    sf::Color inner_color(0, 0, 0, 0);
    sf::Color mid_color(0, 0, 0, static_cast<uint8_t>(max_alpha * 0.85f));
    sf::Color outer_color(0, 0, 0, max_alpha);

    // Two rings: inner-to-mid (sharp), mid-to-outer (gentle)
    sf::VertexArray va(sf::PrimitiveType::Triangles, segments * 12);
    size_t vi = 0;

    for (int i = 0; i < segments; ++i) {
        float a0 = static_cast<float>(i) / segments * pi2;
        float a1 = static_cast<float>(i + 1) / segments * pi2;

        float cos0 = std::cos(a0), sin0 = std::sin(a0);
        float cos1 = std::cos(a1), sin1 = std::sin(a1);

        sf::Vector2f in0{cx + rx_inner * cos0, cy + ry_inner * sin0};
        sf::Vector2f in1{cx + rx_inner * cos1, cy + ry_inner * sin1};
        sf::Vector2f mid0{cx + rx_mid * cos0, cy + ry_mid * sin0};
        sf::Vector2f mid1{cx + rx_mid * cos1, cy + ry_mid * sin1};
        sf::Vector2f out0{cx + outer_r * cos0, cy + outer_r * sin0};
        sf::Vector2f out1{cx + outer_r * cos1, cy + outer_r * sin1};

        // Inner ring: clear to mostly-dark (sharp boundary)
        va[vi++] = {in0, inner_color};
        va[vi++] = {mid0, mid_color};
        va[vi++] = {mid1, mid_color};
        va[vi++] = {in0, inner_color};
        va[vi++] = {mid1, mid_color};
        va[vi++] = {in1, inner_color};

        // Outer ring: mostly-dark to fully dark (gentle)
        va[vi++] = {mid0, mid_color};
        va[vi++] = {out0, outer_color};
        va[vi++] = {out1, outer_color};
        va[vi++] = {mid0, mid_color};
        va[vi++] = {out1, outer_color};
        va[vi++] = {mid1, mid_color};
    }

    active_target_->draw(va);
}

void renderer::draw_fog_gradient(const sf::IntRect& fair, int32_t dw, int32_t dh) {
    constexpr int32_t grad_w = 200;
    constexpr uint8_t max_alpha = 200;
    sf::Color clear_c(0, 0, 0, 0);
    sf::Color dark_c(0, 0, 0, max_alpha);

    float fl = static_cast<float>(fair.position.x);
    float ft = static_cast<float>(fair.position.y);
    float fr = fl + static_cast<float>(fair.size.x);
    float fb = ft + static_cast<float>(fair.size.y);
    float fw = static_cast<float>(dw);
    float fh = static_cast<float>(dh);

    // Gradient boundaries (clamped to screen)
    float gt = std::max(ft - grad_w, 0.0f);
    float gb = std::min(fb + grad_w, fh);
    float gl = std::max(fl - grad_w, 0.0f);
    float gr = std::min(fr + grad_w, fw);

    sf::VertexArray va(sf::PrimitiveType::Triangles);

    auto add_quad = [&](sf::Vector2f v0, sf::Color c0, sf::Vector2f v1, sf::Color c1,
                        sf::Vector2f v2, sf::Color c2, sf::Vector2f v3, sf::Color c3) {
        va.append({v0, c0});
        va.append({v1, c1});
        va.append({v2, c2});
        va.append({v2, c2});
        va.append({v3, c3});
        va.append({v0, c0});
    };

    // Top gradient strip (between fair zone left/right edges)
    add_quad({fl, ft}, clear_c, {fr, ft}, clear_c, {fr, gt}, dark_c, {fl, gt}, dark_c);
    // Bottom gradient strip (between fair zone left/right edges)
    add_quad({fl, fb}, clear_c, {fr, fb}, clear_c, {fr, gb}, dark_c, {fl, gb}, dark_c);
    // Left gradient strip (between fair zone top/bottom edges)
    add_quad({fl, ft}, clear_c, {fl, fb}, clear_c, {gl, fb}, dark_c, {gl, ft}, dark_c);
    // Right gradient strip (between fair zone top/bottom edges)
    add_quad({fr, ft}, clear_c, {fr, fb}, clear_c, {gr, fb}, dark_c, {gr, ft}, dark_c);

    // Corner gradients: diagonal from fair zone corner to gradient corner
    // Top-left corner
    add_quad({fl, ft}, clear_c, {fl, gt}, dark_c, {gl, gt}, dark_c, {gl, ft}, dark_c);
    // Top-right corner
    add_quad({fr, ft}, clear_c, {fr, gt}, dark_c, {gr, gt}, dark_c, {gr, ft}, dark_c);
    // Bottom-left corner
    add_quad({fl, fb}, clear_c, {fl, gb}, dark_c, {gl, gb}, dark_c, {gl, fb}, dark_c);
    // Bottom-right corner
    add_quad({fr, fb}, clear_c, {fr, gb}, dark_c, {gr, gb}, dark_c, {gr, fb}, dark_c);

    active_target_->draw(va);

    // Solid dark beyond gradient
    int32_t igt = static_cast<int32_t>(gt);
    int32_t igb = static_cast<int32_t>(gb);
    int32_t igl = static_cast<int32_t>(gl);
    int32_t igr = static_cast<int32_t>(gr);

    if (igt > 0) draw_rect(0, 0, dw, igt, dark_c);
    if (igb < dh) draw_rect(0, igb, dw, dh - igb, dark_c);
    if (igl > 0) draw_rect(0, igt, igl, igb - igt, dark_c);
    if (igr < dw) draw_rect(igr, igt, dw - igr, igb - igt, dark_c);
}

void renderer::set_targeting_boundary_visible(bool visible) {
    if (visible && !targeting_boundary_visible_) {
        boundary_start_time_ = std::chrono::steady_clock::now();
    }
    targeting_boundary_visible_ = visible;
}

void renderer::set_targeting_boundary_color(sf::Color color) {
    targeting_boundary_color_ = color;
}

void renderer::draw_targeting_boundary(const sf::IntRect& fair) {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - boundary_start_time_).count();

    // Sine pulse: 1.5 second period, alpha oscillates between 40% and 100%
    float pulse = (std::sin(elapsed * 4.18879f) + 1.0f) / 2.0f;  // 0..1, period ~1.5s
    float alpha_factor = 0.4f + 0.6f * pulse;

    sf::Color color = targeting_boundary_color_;
    auto base_a = static_cast<float>(color.a);

    // Outer glow: wider, softer
    float glow_thickness = 3.0f + 2.0f * pulse;
    sf::Color glow_color = color;
    glow_color.a = static_cast<uint8_t>(base_a * alpha_factor * 0.3f);

    sf::RectangleShape glow_rect(
        {static_cast<float>(fair.size.x) + glow_thickness * 2.0f,
         static_cast<float>(fair.size.y) + glow_thickness * 2.0f});
    glow_rect.setPosition(
        {static_cast<float>(fair.position.x) - glow_thickness,
         static_cast<float>(fair.position.y) - glow_thickness});
    glow_rect.setFillColor(sf::Color::Transparent);
    glow_rect.setOutlineColor(glow_color);
    glow_rect.setOutlineThickness(glow_thickness);
    active_target_->draw(glow_rect);

    // Inner border: crisp bright edge
    constexpr float border_thickness = 1.5f;
    sf::Color border_color = color;
    border_color.a = static_cast<uint8_t>(base_a * alpha_factor);

    sf::RectangleShape border_rect(
        {static_cast<float>(fair.size.x), static_cast<float>(fair.size.y)});
    border_rect.setPosition(
        {static_cast<float>(fair.position.x), static_cast<float>(fair.position.y)});
    border_rect.setFillColor(sf::Color::Transparent);
    border_rect.setOutlineColor(border_color);
    border_rect.setOutlineThickness(-border_thickness);
    active_target_->draw(border_rect);
}

// ---------------------------------------------------------------------------
// Drawing (all redirected through active_target_)
// ---------------------------------------------------------------------------

void renderer::draw_sprite(const sprite& spr, int32_t x, int32_t y, uint32_t frame) {
    spr.draw(*active_target_, x, y, frame);
    ++draw_call_count_;
}

void renderer::draw_sprite_alpha(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha) {
    spr.draw_alpha(*active_target_, x, y, frame, alpha);
    ++draw_call_count_;
}

void renderer::draw_sprite_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame) {
    spr.draw_no_color_key(*active_target_, x, y, frame);
    ++draw_call_count_;
}

void renderer::draw_sprite_alpha_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha) {
    spr.draw_alpha_no_color_key(*active_target_, x, y, frame, alpha);
    ++draw_call_count_;
}

// ---------------------------------------------------------------------------
// Additive blending (effect sprites)
// ---------------------------------------------------------------------------

bool renderer::load_additive_shader()
{
    // Shader handles color key transparency directly: multiplies RGB by src.a
    // so color-keyed pixels (alpha=0) produce zero color output regardless of
    // blend mode. Uses One+One blend mode for pure additive compositing.
    static const char* fragment_shader = R"(
        uniform sampler2D texture;
        uniform float alpha;
        uniform vec3 rgb_offset;

        void main()
        {
            vec4 src = texture2D(texture, gl_TexCoord[0].xy);
            vec3 tinted = clamp(src.rgb + rgb_offset, 0.0, 1.0);
            gl_FragColor = vec4(tinted * src.a * alpha, 0.0) * gl_Color;
        }
    )";

    if (!sf::Shader::isAvailable())
    {
        spdlog::warn("Shaders not available - additive blending disabled");
        return false;
    }

    if (!additive_shader_.loadFromMemory(fragment_shader, sf::Shader::Type::Fragment))
    {
        spdlog::error("Failed to load additive blending shader");
        return false;
    }

    spdlog::info("Additive blending shader loaded");
    return true;
}

void renderer::draw_sprite_additive(const sprite& spr, int32_t x, int32_t y, uint32_t frame)
{
    draw_sprite_additive_alpha(spr, x, y, frame, 1.0f);
}

void renderer::draw_sprite_additive_alpha(const sprite& spr, int32_t x, int32_t y,
                                           uint32_t frame, float alpha)
{
    // Lazy-load shader on first use
    if (!additive_shader_init_attempted_)
    {
        additive_shader_init_attempted_ = true;
        additive_shader_loaded_ = load_additive_shader();
    }

    // Fallback to standard alpha if shader unavailable
    if (!additive_shader_loaded_)
    {
        draw_sprite_alpha(spr, x, y, frame, alpha);
        return;
    }

    if (!spr.ensure_loaded() || frame >= spr.frame_count())
    {
        return;
    }

    const auto& f = spr.get_frame(frame);
    sf::Sprite sf_spr(spr.texture(), f.source_rect);
    sf_spr.setPosition({
        static_cast<float>(x + f.pivot_x),
        static_cast<float>(y + f.pivot_y)
    });

    // One+One blend: shader already handles alpha (color key + opacity),
    // so blend mode just adds shader output directly to destination.
    static const sf::BlendMode additive_blend(
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add,
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add
    );

    sf::RenderStates states;
    states.blendMode = additive_blend;
    additive_shader_.setUniform("texture", sf::Shader::CurrentTexture);
    additive_shader_.setUniform("alpha", alpha * additive_intensity_);
    additive_shader_.setUniform("rgb_offset", sf::Glsl::Vec3(0.0f, 0.0f, 0.0f));
    states.shader = &additive_shader_;

    active_target_->draw(sf_spr, states);
    ++draw_call_count_;
}

void renderer::draw_sprite_additive_tinted(const sprite& spr, int32_t x, int32_t y,
                                            uint32_t frame, float alpha,
                                            int16_t r_offset, int16_t g_offset, int16_t b_offset)
{
    // Lazy-load shader on first use
    if (!additive_shader_init_attempted_)
    {
        additive_shader_init_attempted_ = true;
        additive_shader_loaded_ = load_additive_shader();
    }

    // Fallback to standard alpha if shader unavailable
    if (!additive_shader_loaded_)
    {
        draw_sprite_alpha(spr, x, y, frame, alpha);
        return;
    }

    if (!spr.ensure_loaded() || frame >= spr.frame_count())
    {
        return;
    }

    const auto& f = spr.get_frame(frame);
    sf::Sprite sf_spr(spr.texture(), f.source_rect);
    sf_spr.setPosition({
        static_cast<float>(x + f.pivot_x),
        static_cast<float>(y + f.pivot_y)
    });

    static const sf::BlendMode additive_blend(
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add,
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add
    );

    sf::RenderStates states;
    states.blendMode = additive_blend;
    additive_shader_.setUniform("texture", sf::Shader::CurrentTexture);
    additive_shader_.setUniform("alpha", alpha * additive_intensity_);
    additive_shader_.setUniform("rgb_offset", sf::Glsl::Vec3(
        static_cast<float>(r_offset) / 255.0f,
        static_cast<float>(g_offset) / 255.0f,
        static_cast<float>(b_offset) / 255.0f
    ));
    states.shader = &additive_shader_;

    active_target_->draw(sf_spr, states);
    ++draw_call_count_;
}

bool renderer::load_tint_shader()
{
    // Additive RGB offset shader for color tinting (e.g. hair color).
    // Adds a uniform color_offset to each non-transparent pixel.
    static const char* fragment_shader = R"(
        uniform sampler2D texture;
        uniform vec3 color_offset;

        void main()
        {
            vec4 src = texture2D(texture, gl_TexCoord[0].xy);
            if (src.a > 0.0)
            {
                src.rgb = clamp(src.rgb + color_offset, 0.0, 1.0);
            }
            gl_FragColor = src * gl_Color;
        }
    )";

    if (!sf::Shader::isAvailable())
    {
        spdlog::warn("Shaders not available - color tinting disabled");
        return false;
    }

    if (!tint_shader_.loadFromMemory(fragment_shader, sf::Shader::Type::Fragment))
    {
        spdlog::error("Failed to load color tint shader");
        return false;
    }

    spdlog::info("Color tint shader loaded");
    return true;
}

void renderer::draw_sprite_tinted(const sprite& spr, int32_t x, int32_t y, uint32_t frame,
                                   float r_offset, float g_offset, float b_offset)
{
    if (!tint_shader_init_attempted_)
    {
        tint_shader_init_attempted_ = true;
        tint_shader_loaded_ = load_tint_shader();
    }

    if (!tint_shader_loaded_)
    {
        draw_sprite(spr, x, y, frame);
        return;
    }

    if (!spr.ensure_loaded() || frame >= spr.frame_count())
    {
        return;
    }

    const auto& f = spr.get_frame(frame);
    sf::Sprite sf_spr(spr.texture(), f.source_rect);
    sf_spr.setPosition({
        static_cast<float>(x + f.pivot_x),
        static_cast<float>(y + f.pivot_y)
    });

    sf::RenderStates states;
    tint_shader_.setUniform("texture", sf::Shader::CurrentTexture);
    tint_shader_.setUniform("color_offset", sf::Glsl::Vec3(r_offset, g_offset, b_offset));
    states.shader = &tint_shader_;

    active_target_->draw(sf_spr, states);
    ++draw_call_count_;
}

void renderer::draw_texture(const sf::Texture& texture, int32_t x, int32_t y) {
    sf::Sprite spr(texture);
    spr.setPosition({static_cast<float>(x), static_cast<float>(y)});
    active_target_->draw(spr);
    ++draw_call_count_;
}

void renderer::draw_texture(const sf::Texture& texture, int32_t x, int32_t y, const sf::IntRect& rect) {
    sf::Sprite spr(texture, rect);
    spr.setPosition({static_cast<float>(x), static_cast<float>(y)});
    active_target_->draw(spr);
    ++draw_call_count_;
}

bool renderer::load_font(std::string_view path) {
    if (font_.openFromFile(std::string(path))) {
        font_loaded_ = true;

        // Initialize text renderer with the loaded font and active target
        text_renderer_.initialize(font_, *active_target_);

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
    active_target_->draw(sf_text);
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
    active_target_->draw(sf_text);
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

    active_target_->draw(rect);
}

void renderer::draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, sf::Color color) {
    std::array<sf::Vertex, 2> line = {
        sf::Vertex{{static_cast<float>(x1), static_cast<float>(y1)}, color},
        sf::Vertex{{static_cast<float>(x2), static_cast<float>(y2)}, color}
    };
    active_target_->draw(line.data(), 2, sf::PrimitiveType::Lines);
}

void renderer::push_scissor(int32_t x, int32_t y, int32_t w, int32_t h) {
    // Flush SFML's render queue before changing OpenGL state
    if (rendering_scene_ && scene_rt_) {
        (void)scene_rt_->setActive(true);
    } else {
        (void)window_.setActive(true);
    }

    // OpenGL Y coordinate is flipped (0 at bottom)
    // Use the correct target height for the Y-flip
    uint32_t target_h = rendering_scene_ ? internal_height_ : height_;
    int32_t gl_y = static_cast<int32_t>(target_h) - y - h;

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, gl_y, w, h);
}

void renderer::pop_scissor() {
    glDisable(GL_SCISSOR_TEST);
}

void renderer::set_zoom_view(float zoom_level, float center_x, float center_y) {
    sf::View view = active_target_->getDefaultView();
    view.setCenter({center_x, center_y});
    view.zoom(zoom_level);
    active_target_->setView(view);
}

void renderer::reset_to_default_view() {
    active_target_->setView(active_target_->getDefaultView());
}

void renderer::set_topmost(bool topmost) {
    if (!window_.isOpen()) return;

#ifdef _WIN32
    auto hwnd = reinterpret_cast<HWND>(window_.getNativeHandle());
    SetWindowPos(hwnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
    auto xwindow = static_cast<Window>(window_.getNativeHandle());
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wm_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = xwindow;
    event.xclient.message_type = wm_state;
    event.xclient.format = 32;
    event.xclient.data.l[0] = topmost ? 1 : 0;  // _NET_WM_STATE_ADD or _NET_WM_STATE_REMOVE
    event.xclient.data.l[1] = static_cast<long>(wm_above);
    event.xclient.data.l[2] = 0;

    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(display);
    XCloseDisplay(display);
#endif

    spdlog::debug("Window topmost: {}", topmost);
}

void renderer::on_focus_changed(bool has_focus) {
    if (!borderless_) return;

    // When a borderless window loses focus, remove topmost so the user can
    // interact with other windows. Restore topmost when focus returns.
    set_topmost(has_focus);
}

} // namespace hb
