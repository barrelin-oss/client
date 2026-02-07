#pragma once

#include "graphics/text_renderer.hpp"
#include <SFML/Graphics.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace hb {

class sprite;

// Server-controlled rendering mode
enum class view_mode : uint8_t
{
    special = 0,   // Current behavior - native resolution, zoom, no restrictions
    scaled = 1,    // Fixed internal resolution scaled up to display
    extended = 2,  // Native resolution but entities only render within fair zone
};

// Player-configurable aspect ratio for scaled mode
enum class aspect_mode : uint8_t
{
    letterbox = 0, // Uniform scale with black bars
    stretch = 1,   // Independent x/y scaling to fill display
};

// Player-configurable scaling filter for scaled mode
enum class scale_filter : uint8_t
{
    nearest = 0,   // Crisp pixel scaling
    bilinear = 1,  // Smooth interpolation
};

// Fog overlay style for extended mode (visual treatment of area outside fair zone)
enum class fog_style : uint8_t
{
    solid = 0,      // Hard-edged dark rectangles
    tile_fog = 1,   // Tile-grid-aligned boundary
    vignette = 2,   // Elliptical gradient darkening
    gradient = 3,   // Linear gradient fade from fair zone edge
};

class renderer {
public:
    bool initialize(uint32_t width, uint32_t height, bool fullscreen,
                    bool borderless = false, int32_t monitor_x = 0, int32_t monitor_y = 0);
    void shutdown();

    void begin_frame();
    void end_frame();

    // View mode system
    void set_view_mode(view_mode mode);
    void set_internal_resolution(uint32_t w, uint32_t h);
    void begin_scene();  // Scaled: redirect to scene_rt_. Others: no-op.
    void end_scene();    // Scaled: composite. Extended: fog overlay. Special: no-op.
    void begin_ui();     // Ensure drawing to window at native resolution.

    view_mode current_view_mode() const { return view_mode_; }

    // Scene dimensions (what game world rendering uses)
    // Scaled: internal resolution. Extended/Special: display resolution.
    uint32_t scene_width() const;
    uint32_t scene_height() const;

    // Display dimensions (always window size)
    uint32_t display_width() const { return width_; }
    uint32_t display_height() const { return height_; }

    // Interaction dimensions (what the player can see/target - used for view range)
    // Special: display resolution. Scaled/Extended: fair zone (internal resolution).
    uint32_t interaction_width() const;
    uint32_t interaction_height() const;

    // Coordinate mapping between display and scene space
    std::pair<int32_t, int32_t> display_to_scene(int32_t x, int32_t y) const;
    std::pair<int32_t, int32_t> scene_to_display(int32_t x, int32_t y) const;

    // Fair zone bounds in screen coordinates (for extended mode culling/targeting)
    sf::IntRect fair_bounds() const;
    bool is_in_fair_zone(int32_t display_x, int32_t display_y) const;

    // View mode settings
    void set_aspect_mode(aspect_mode mode) { aspect_mode_ = mode; }
    void set_scale_filter(scale_filter filter) { scale_filter_ = filter; }
    void set_ui_scale(float scale) { ui_scale_ = scale; }
    aspect_mode current_aspect_mode() const { return aspect_mode_; }
    scale_filter current_scale_filter() const { return scale_filter_; }
    float ui_scale() const { return ui_scale_; }

    // Fog overlay style (extended mode only)
    void set_fog_style(fog_style style);
    fog_style current_fog_style() const { return fog_style_; }
    void set_fog_camera_info(int32_t camera_x, int32_t camera_y, float zoom);

    // Targeting boundary (pulsing outline at fair zone edge, for ranged interactions)
    void set_targeting_boundary_visible(bool visible);
    void set_targeting_boundary_color(sf::Color color);
    bool is_targeting_boundary_visible() const { return targeting_boundary_visible_; }

    // Sprite drawing (with color key transparency)
    void draw_sprite(const sprite& spr, int32_t x, int32_t y, uint32_t frame = 0);
    void draw_sprite_alpha(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha);

    // Sprite drawing without color key (for backgrounds, etc.)
    void draw_sprite_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame = 0);
    void draw_sprite_alpha_no_color_key(const sprite& spr, int32_t x, int32_t y, uint32_t frame, float alpha);

    // Raw texture drawing
    void draw_texture(const sf::Texture& texture, int32_t x, int32_t y);
    void draw_texture(const sf::Texture& texture, int32_t x, int32_t y, const sf::IntRect& rect);

    // Text rendering - static (no effects)
    bool load_font(std::string_view path);
    void draw_text(std::string_view text, int32_t x, int32_t y, sf::Color color = sf::Color::White);
    void draw_text(std::string_view text, int32_t x, int32_t y, sf::Color color, uint32_t size);
    void draw_text_outlined(std::string_view text, int32_t x, int32_t y,
                            sf::Color color, sf::Color outline_color,
                            uint32_t size = 12, float outline_thickness = 1.0f);

    // Unified text renderer (for animated/shader effects)
    text_renderer& text() { return text_renderer_; }
    const text_renderer& text() const { return text_renderer_; }

    // Primitives
    void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, sf::Color color, bool filled = true);
    void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, sf::Color color);

    // Scissor clipping (restricts rendering to a rectangular region)
    void push_scissor(int32_t x, int32_t y, int32_t w, int32_t h);
    void pop_scissor();

    // Resolution change
    bool set_resolution(uint32_t width, uint32_t height, bool fullscreen,
                        bool borderless = false, int32_t monitor_x = 0, int32_t monitor_y = 0);

    // View control for zoom
    // zoom_level: 1.0 = normal, >1 = zoomed out, <1 = zoomed in
    // anchor_x/y: screen position to zoom toward (point stays fixed on screen)
    void set_zoom_view(float zoom_level, float anchor_x, float anchor_y);
    void reset_to_default_view();

    // Accessors
    sf::RenderWindow& window() { return window_; }
    const sf::RenderWindow& window() const { return window_; }
    sf::RenderTarget& active_target() { return *active_target_; }
    bool is_open() const { return window_.isOpen(); }

    // Legacy width()/height() - returns scene dimensions for backward compatibility
    uint32_t width() const { return scene_width(); }
    uint32_t height() const { return scene_height(); }

    // Draw call tracking
    uint32_t draw_call_count() const { return draw_call_count_; }
    void reset_draw_call_count() { draw_call_count_ = 0; }

    // Borderless topmost management
    bool is_borderless() const { return borderless_; }
    void set_topmost(bool topmost);

    // Call when window gains/loses focus to toggle topmost for borderless windows
    void on_focus_changed(bool has_focus);

private:
    void create_scene_rt();
    void destroy_scene_rt();

    // Fog overlay rendering methods
    void draw_fog_solid(const sf::IntRect& fair, int32_t dw, int32_t dh);
    void draw_fog_tile(const sf::IntRect& fair, int32_t dw, int32_t dh);
    void draw_fog_vignette(const sf::IntRect& fair, int32_t dw, int32_t dh);
    void draw_fog_gradient(const sf::IntRect& fair, int32_t dw, int32_t dh);
    void draw_targeting_boundary(const sf::IntRect& fair);

    sf::RenderWindow window_;
    sf::Font font_;
    bool font_loaded_ = false;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool borderless_ = false;
    uint32_t draw_call_count_ = 0;

    // View mode system
    view_mode view_mode_ = view_mode::scaled;
    std::unique_ptr<sf::RenderTexture> scene_rt_;
    uint32_t internal_width_ = 800;
    uint32_t internal_height_ = 600;
    aspect_mode aspect_mode_ = aspect_mode::letterbox;
    scale_filter scale_filter_ = scale_filter::nearest;
    float ui_scale_ = 1.0f;
    sf::RenderTarget* active_target_ = nullptr;
    bool rendering_scene_ = false;

    // Fog overlay system
    fog_style fog_style_ = fog_style::solid;
    int32_t fog_camera_x_ = 0;
    int32_t fog_camera_y_ = 0;
    float fog_zoom_ = 1.0f;

    // Targeting boundary
    bool targeting_boundary_visible_ = false;
    sf::Color targeting_boundary_color_{64, 192, 255, 255};  // Default: soft blue
    std::chrono::steady_clock::time_point boundary_start_time_;

    text_renderer text_renderer_;
};

} // namespace hb
