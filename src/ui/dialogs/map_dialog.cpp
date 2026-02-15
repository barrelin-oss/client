#include "ui/dialogs/map_dialog.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <format>
#include <cmath>

namespace hb
{

map_dialog::map_dialog() : dialog(dialog_type::map)
{
    set_title("World Map");
    set_bounds({100, 60, 440, 380});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void map_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

void map_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    int32_t x = bounds_.x + 10;
    int32_t y = bounds_.y + 32;

    // Map name and coordinates
    rend.draw_text(map_name_, x, y, sf::Color(200, 200, 100));
    y += 18;

    std::string coords = std::format("Position: ({}, {})", player_x_, player_y_);
    rend.draw_text(coords, x, y, sf::Color(180, 180, 180), 11);
    y += 18;

    // Map display area
    map_area_x_ = x;
    map_area_y_ = y;
    map_area_w_ = bounds_.width - 20;
    map_area_h_ = 240;

    render_map_area(rend, map_area_x_, map_area_y_, map_area_w_, map_area_h_);
    y += map_area_h_ + 8;

    // Legend
    render_legend(rend, x, y);

    // Zoom controls
    int32_t zoom_x = bounds_.x + bounds_.width - 80;
    int32_t zoom_y = bounds_.y + bounds_.height - 35;

    rend.draw_rect(zoom_x, zoom_y, 30, 24, sf::Color(50, 50, 65), true);
    rend.draw_rect(zoom_x, zoom_y, 30, 24, sf::Color(80, 80, 100), false);
    rend.draw_text("-", zoom_x + 11, zoom_y + 4, sf::Color::White);

    rend.draw_rect(zoom_x + 35, zoom_y, 30, 24, sf::Color(50, 50, 65), true);
    rend.draw_rect(zoom_x + 35, zoom_y, 30, 24, sf::Color(80, 80, 100), false);
    rend.draw_text("+", zoom_x + 45, zoom_y + 4, sf::Color::White);

    // Zoom level indicator
    std::string zoom_text = std::format("{:.0f}%", zoom_level_ * 100.0f);
    rend.draw_text(zoom_text, zoom_x - 40, zoom_y + 6, sf::Color(150, 150, 150), 11);

    // Hovered location tooltip
    if (hovered_location_.has_value())
    {
        auto [hx, hy] = hovered_location_.value();
        std::string tooltip = std::format("({}, {})", hx, hy);
        rend.draw_rect(map_area_x_ + map_area_w_ - 60, map_area_y_ + 4, 55, 16, sf::Color(0, 0, 0, 180), true);
        rend.draw_text(tooltip, map_area_x_ + map_area_w_ - 58, map_area_y_ + 6, sf::Color::White, 10);
    }
}

void map_dialog::render_map_area(renderer& rend, int32_t x, int32_t y, int32_t width, int32_t height)
{
    // Background
    rend.draw_rect(x, y, width, height, sf::Color(20, 30, 40), true);
    rend.draw_rect(x, y, width, height, sf::Color(60, 70, 80), false);

    // Grid lines
    int32_t grid_spacing = static_cast<int32_t>(32 * zoom_level_);
    if (grid_spacing > 8)
    {
        sf::Color grid_color(40, 50, 60);
        for (int32_t gx = x; gx < x + width; gx += grid_spacing)
        {
            rend.draw_line(gx, y, gx, y + height, grid_color);
        }
        for (int32_t gy = y; gy < y + height; gy += grid_spacing)
        {
            rend.draw_line(x, gy, x + width, gy, grid_color);
        }
    }

    // Render markers
    for (const auto& marker : markers_)
    {
        bool should_show = false;
        switch (marker.type)
        {
        case 0:
            should_show = true;
            break; // Player always shown
        case 1:
            should_show = show_party_;
            break;
        case 2:
            should_show = show_enemies_;
            break;
        case 3:
            should_show = show_npcs_;
            break;
        case 4:
            should_show = show_teleports_;
            break;
        }

        if (!should_show)
            continue;

        auto [mx, my] = world_to_map(marker.x, marker.y, x, y, width, height);
        if (mx >= x && mx < x + width && my >= y && my < y + height)
        {
            int32_t size = (marker.type == 0) ? 6 : 4; // Player marker is larger

            rend.draw_rect(mx - size / 2, my - size / 2, size, size, marker.color, true);

            // Draw name for player and party members
            if (marker.type <= 1 && !marker.name.empty())
            {
                rend.draw_text(
                    marker.name, mx - static_cast<int32_t>(marker.name.length() * 2), my - 12, marker.color, 9);
            }
        }
    }

    // Draw player position (always on top)
    auto [px, py] = world_to_map(player_x_, player_y_, x, y, width, height);
    if (px >= x && px < x + width && py >= y && py < y + height)
    {
        // Player marker as a small triangle pointing up
        rend.draw_rect(px - 3, py - 3, 7, 7, sf::Color(100, 200, 100), true);
        rend.draw_rect(px - 3, py - 3, 7, 7, sf::Color::White, false);
    }
}

void map_dialog::render_legend(renderer& rend, int32_t x, int32_t y)
{
    rend.draw_text("Legend:", x, y, sf::Color(180, 180, 180), 11);

    int32_t legend_x = x + 50;
    int32_t spacing = 65;

    // Player
    rend.draw_rect(legend_x, y + 2, 8, 8, sf::Color(100, 200, 100), true);
    rend.draw_text("You", legend_x + 12, y, sf::Color(100, 200, 100), 10);
    legend_x += spacing;

    // Party
    if (show_party_)
    {
        rend.draw_rect(legend_x, y + 2, 6, 6, sf::Color(100, 150, 255), true);
        rend.draw_text("Party", legend_x + 10, y, sf::Color(100, 150, 255), 10);
        legend_x += spacing;
    }

    // Enemies
    if (show_enemies_)
    {
        rend.draw_rect(legend_x, y + 2, 6, 6, sf::Color(255, 100, 100), true);
        rend.draw_text("Enemy", legend_x + 10, y, sf::Color(255, 100, 100), 10);
        legend_x += spacing;
    }

    // NPCs
    if (show_npcs_)
    {
        rend.draw_rect(legend_x, y + 2, 6, 6, sf::Color(255, 220, 100), true);
        rend.draw_text("NPC", legend_x + 10, y, sf::Color(255, 220, 100), 10);
        legend_x += spacing;
    }

    // Teleports
    if (show_teleports_)
    {
        rend.draw_rect(legend_x, y + 2, 6, 6, sf::Color(200, 100, 255), true);
        rend.draw_text("Warp", legend_x + 10, y, sf::Color(200, 100, 255), 10);
    }
}

std::pair<int32_t, int32_t> map_dialog::world_to_map(
    int32_t world_x, int32_t world_y, int32_t area_x, int32_t area_y, int32_t area_w, int32_t area_h) const
{
    // Calculate visible world area based on zoom and pan
    float visible_world_w = map_width_ / zoom_level_;
    float visible_world_h = map_height_ / zoom_level_;

    // Center on player with pan offset
    float center_x = static_cast<float>(player_x_) + pan_x_;
    float center_y = static_cast<float>(player_y_) + pan_y_;

    float world_left = center_x - visible_world_w / 2.0f;
    float world_top = center_y - visible_world_h / 2.0f;

    // Convert world position to map area position
    float rel_x = (world_x - world_left) / visible_world_w;
    float rel_y = (world_y - world_top) / visible_world_h;

    int32_t map_x = area_x + static_cast<int32_t>(rel_x * area_w);
    int32_t map_y = area_y + static_cast<int32_t>(rel_y * area_h);

    return {map_x, map_y};
}

std::optional<std::pair<int32_t, int32_t>>
map_dialog::map_to_world(int32_t mx, int32_t my, int32_t area_x, int32_t area_y, int32_t area_w, int32_t area_h) const
{
    if (mx < area_x || mx >= area_x + area_w || my < area_y || my >= area_y + area_h)
    {
        return std::nullopt;
    }

    float visible_world_w = map_width_ / zoom_level_;
    float visible_world_h = map_height_ / zoom_level_;

    float center_x = static_cast<float>(player_x_) + pan_x_;
    float center_y = static_cast<float>(player_y_) + pan_y_;

    float world_left = center_x - visible_world_w / 2.0f;
    float world_top = center_y - visible_world_h / 2.0f;

    float rel_x = static_cast<float>(mx - area_x) / area_w;
    float rel_y = static_cast<float>(my - area_y) / area_h;

    int32_t world_x = static_cast<int32_t>(world_left + rel_x * visible_world_w);
    int32_t world_y = static_cast<int32_t>(world_top + rel_y * visible_world_h);

    return std::make_pair(world_x, world_y);
}

bool map_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    // Zoom buttons
    int32_t zoom_x = bounds_.x + bounds_.width - 80;
    int32_t zoom_y = bounds_.y + bounds_.height - 35;

    ui_rect zoom_out_btn{zoom_x, zoom_y, 30, 24};
    if (zoom_out_btn.contains(x, y) && btn == sf::Mouse::Button::Left)
    {
        zoom_out();
        return true;
    }

    ui_rect zoom_in_btn{zoom_x + 35, zoom_y, 30, 24};
    if (zoom_in_btn.contains(x, y) && btn == sf::Mouse::Button::Left)
    {
        zoom_in();
        return true;
    }

    // Map area click
    ui_rect map_area{map_area_x_, map_area_y_, map_area_w_, map_area_h_};
    if (map_area.contains(x, y))
    {
        if (btn == sf::Mouse::Button::Left)
        {
            auto world_pos = map_to_world(x, y, map_area_x_, map_area_y_, map_area_w_, map_area_h_);
            if (world_pos.has_value() && on_click_location_)
            {
                on_click_location_(world_pos->first, world_pos->second);
            }
        }
        else if (btn == sf::Mouse::Button::Right)
        {
            // Start panning
            dragging_ = true;
            drag_start_x_ = x;
            drag_start_y_ = y;
        }
        return true;
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool map_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;

    // Update hovered location
    auto world_pos = map_to_world(x, y, map_area_x_, map_area_y_, map_area_w_, map_area_h_);
    hovered_location_ = world_pos;

    // Handle panning
    if (dragging_)
    {
        float scale = map_width_ / (map_area_w_ * zoom_level_);
        pan_x_ -= static_cast<int32_t>((x - drag_start_x_) * scale);
        pan_y_ -= static_cast<int32_t>((y - drag_start_y_) * scale);
        drag_start_x_ = x;
        drag_start_y_ = y;
        return true;
    }

    return dialog::handle_mouse_move(x, y);
}

bool map_dialog::handle_key_press(sf::Keyboard::Key key)
{
    if (!visible_)
        return false;

    if (key == sf::Keyboard::Key::Add || key == sf::Keyboard::Key::Equal)
    {
        zoom_in();
        return true;
    }

    if (key == sf::Keyboard::Key::Subtract || key == sf::Keyboard::Key::Hyphen)
    {
        zoom_out();
        return true;
    }

    if (key == sf::Keyboard::Key::Home)
    {
        reset_zoom();
        pan_x_ = 0;
        pan_y_ = 0;
        return true;
    }

    return false;
}

void map_dialog::zoom_in()
{
    zoom_level_ = std::min(zoom_level_ * 1.25f, 4.0f);
}

void map_dialog::zoom_out()
{
    zoom_level_ = std::max(zoom_level_ / 1.25f, 0.25f);
}

} // namespace hb
