#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <vector>
#include <string>
#include <optional>

namespace hb
{

// Map marker structure
struct map_marker
{
    int32_t x = 0;
    int32_t y = 0;
    std::string name;
    sf::Color color = sf::Color::White;
    uint8_t type = 0; // 0=player, 1=party, 2=enemy, 3=npc, 4=teleport
};

// Map dialog - for world map / minimap display
class map_dialog : public dialog
{
public:
    map_dialog();
    ~map_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;
    bool handle_key_press(sf::Keyboard::Key key) override;

    // Map info
    void set_map_name(std::string_view name) { map_name_ = name; }
    void set_map_size(int32_t width, int32_t height)
    {
        map_width_ = width;
        map_height_ = height;
    }

    // Player position
    void set_player_position(int32_t x, int32_t y)
    {
        player_x_ = x;
        player_y_ = y;
    }

    // Markers
    void clear_markers() { markers_.clear(); }
    void add_marker(const map_marker& marker) { markers_.push_back(marker); }
    void set_markers(std::vector<map_marker> markers) { markers_ = std::move(markers); }

    // Display options
    void set_show_party(bool show) { show_party_ = show; }
    void set_show_enemies(bool show) { show_enemies_ = show; }
    void set_show_npcs(bool show) { show_npcs_ = show; }
    void set_show_teleports(bool show) { show_teleports_ = show; }

    // Zoom
    void zoom_in();
    void zoom_out();
    void reset_zoom() { zoom_level_ = 1.0f; }

    // Callbacks
    using location_callback = std::function<void(int32_t x, int32_t y)>;
    void set_on_click_location(location_callback callback) { on_click_location_ = std::move(callback); }

private:
    void render_map_area(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height);
    void render_legend(renderer& rend, int32_t x, int32_t y);
    std::pair<int32_t, int32_t> world_to_map(int32_t world_x,
                                             int32_t world_y,
                                             int32_t map_area_x,
                                             int32_t map_area_y,
                                             int32_t map_area_w,
                                             int32_t map_area_h) const;
    std::optional<std::pair<int32_t, int32_t>> map_to_world(
        int32_t mx, int32_t my, int32_t map_area_x, int32_t map_area_y, int32_t map_area_w, int32_t map_area_h) const;

    std::string map_name_;
    int32_t map_width_ = 512;
    int32_t map_height_ = 512;

    int32_t player_x_ = 0;
    int32_t player_y_ = 0;

    std::vector<map_marker> markers_;

    bool show_party_ = true;
    bool show_enemies_ = true;
    bool show_npcs_ = true;
    bool show_teleports_ = true;

    float zoom_level_ = 1.0f;
    int32_t pan_x_ = 0;
    int32_t pan_y_ = 0;
    bool dragging_ = false;
    int32_t drag_start_x_ = 0;
    int32_t drag_start_y_ = 0;

    std::optional<std::pair<int32_t, int32_t>> hovered_location_;

    int32_t map_area_x_ = 0;
    int32_t map_area_y_ = 0;
    int32_t map_area_w_ = 0;
    int32_t map_area_h_ = 0;

    location_callback on_click_location_;
};

} // namespace hb
