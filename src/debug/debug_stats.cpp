#include "debug/debug_stats.hpp"
#include "graphics/renderer.hpp"
#include <spdlog/spdlog.h>
#include <cstdio>

namespace hb {
namespace debug {

debug_stats& debug_stats::instance()
{
    static debug_stats instance;
    return instance;
}

void debug_stats::update(float delta_time)
{
    // Store delta time for display
    delta_time_ms_ = delta_time * 1000.0f;

    // Update FPS counter
    frame_count_++;
    fps_timer_ += delta_time;

    if (fps_timer_ >= 1.0f)
    {
        current_fps_ = static_cast<float>(frame_count_) / fps_timer_;
        frame_count_ = 0;
        fps_timer_ = 0.0f;
    }

    // Update network message rate
    network_stats_timer_ += delta_time;
    if (network_stats_timer_ >= 1.0f)
    {
        messages_received_per_sec_ = messages_received_counter_;
        messages_sent_per_sec_ = messages_sent_counter_;
        messages_received_counter_ = 0;
        messages_sent_counter_ = 0;
        network_stats_timer_ = 0.0f;
    }

    // Count messages for per-second calculation
    if (messages_received_ > 0)
    {
        messages_received_counter_ += messages_received_;
        messages_received_ = 0;
    }
    if (messages_sent_ > 0)
    {
        messages_sent_counter_ += messages_sent_;
        messages_sent_ = 0;
    }
}

void debug_stats::render_section(renderer& rend, int32_t& y, const char* title)
{
    int32_t x = padding_ + 8;
    rend.draw_text(title, x, y, sf::Color(140, 180, 220), 10);
    y += line_height_;
}

void debug_stats::render(renderer& rend)
{
    if (!visible_)
    {
        return;
    }

    // Calculate box height based on content (estimate)
    int32_t box_height = padding_ * 2 + line_height_ * 24 + section_spacing_ * 7;

    // Draw semi-transparent background with soft shadow effect
    // Shadow
    rend.draw_rect(padding_ + 3, padding_ + 3, box_width_, box_height,
                   sf::Color(0, 0, 0, 80), true);
    // Background
    rend.draw_rect(padding_, padding_, box_width_, box_height,
                   sf::Color(20, 20, 30, 220), true);
    // Border
    rend.draw_rect(padding_, padding_, box_width_, box_height,
                   sf::Color(60, 60, 80, 220), false);

    int32_t x = padding_ + 8;
    int32_t y = padding_ + 6;
    char buf[128];

    // Title
    rend.draw_text("Debug Stats", x, y, sf::Color(200, 200, 220), 11);
    rend.draw_text("(Alt+`)", x + 80, y, sf::Color(120, 120, 140), 9);
    y += line_height_ + 2;

    // Separator
    rend.draw_line(x, y, x + box_width_ - 16, y, sf::Color(60, 60, 80));
    y += section_spacing_;

    // === Performance ===
    render_section(rend, y, "Performance");

    // FPS
    snprintf(buf, sizeof(buf), "FPS: %.1f", current_fps_);
    sf::Color fps_color = current_fps_ >= 55.0f ? sf::Color(100, 200, 100) :
                          current_fps_ >= 30.0f ? sf::Color(200, 200, 100) :
                                                  sf::Color(200, 100, 100);
    rend.draw_text(buf, x + 8, y, fps_color, 10);
    y += line_height_;

    // Delta time
    snprintf(buf, sizeof(buf), "Frame: %.2f ms", delta_time_ms_);
    sf::Color dt_color = delta_time_ms_ <= 17.0f ? sf::Color(100, 200, 100) :
                         delta_time_ms_ <= 33.0f ? sf::Color(200, 200, 100) :
                                                   sf::Color(200, 100, 100);
    rend.draw_text(buf, x + 8, y, dt_color, 10);
    y += line_height_ + section_spacing_;

    // === Network ===
    render_section(rend, y, "Network");

    // Connection status
    const char* conn_status = network_connected_ ? "Connected" : "Disconnected";
    sf::Color conn_color = network_connected_ ? sf::Color(100, 200, 100) : sf::Color(200, 100, 100);
    snprintf(buf, sizeof(buf), "Status: %s", conn_status);
    rend.draw_text(buf, x + 8, y, conn_color, 10);
    y += line_height_;

    // Ping
    if (network_connected_ && ping_ms_ > 0)
    {
        snprintf(buf, sizeof(buf), "Ping: %d ms", ping_ms_);
        sf::Color ping_color = ping_ms_ <= 50 ? sf::Color(100, 200, 100) :
                               ping_ms_ <= 150 ? sf::Color(200, 200, 100) :
                                                 sf::Color(200, 100, 100);
        rend.draw_text(buf, x + 8, y, ping_color, 10);
    }
    else
    {
        rend.draw_text("Ping: --", x + 8, y, sf::Color(150, 150, 170), 10);
    }
    y += line_height_;

    // Messages per second
    snprintf(buf, sizeof(buf), "Recv: %d/s  Sent: %d/s", messages_received_per_sec_, messages_sent_per_sec_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === Rendering ===
    render_section(rend, y, "Rendering");

    snprintf(buf, sizeof(buf), "Sprites: %d  Tiles: %d", sprites_rendered_, tiles_rendered_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === Assets ===
    render_section(rend, y, "Assets");

    snprintf(buf, sizeof(buf), "Sprite cache: %d", sprite_cache_count_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "PAK files: %d", pak_files_loaded_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === World ===
    render_section(rend, y, "World");

    // Map name
    if (!map_name_.empty())
    {
        snprintf(buf, sizeof(buf), "Map: %s", map_name_.c_str());
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
        y += line_height_;
    }

    // Camera bounds
    snprintf(buf, sizeof(buf), "Camera: (%d,%d) - (%d,%d)",
             camera_left_, camera_top_, camera_right_, camera_bottom_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    // Entity count
    snprintf(buf, sizeof(buf), "Entities: %d", entity_count_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === Player ===
    render_section(rend, y, "Player");

    snprintf(buf, sizeof(buf), "Tile: (%d, %d)", player_tile_x_, player_tile_y_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "World: (%d, %d)", player_world_x_, player_world_y_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === Input ===
    render_section(rend, y, "Input");

    snprintf(buf, sizeof(buf), "Mouse screen: (%d, %d)", mouse_screen_x_, mouse_screen_y_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Mouse world: (%d, %d)", mouse_world_x_, mouse_world_y_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Mouse tile: (%d, %d)", mouse_tile_x_, mouse_tile_y_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    if (!hovered_entity_.empty())
    {
        snprintf(buf, sizeof(buf), "Hover: %s", hovered_entity_.c_str());
        rend.draw_text(buf, x + 8, y, sf::Color(200, 180, 100), 10);
        y += line_height_;
    }

    y += section_spacing_;

    // === Game State ===
    render_section(rend, y, "Game State");

    if (!game_state_.empty())
    {
        snprintf(buf, sizeof(buf), "State: %s", game_state_.c_str());
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
        y += line_height_;
    }

    // Combat mode
    const char* combat_str = combat_mode_ ? "Attack" : "Peace";
    sf::Color combat_color = combat_mode_ ? sf::Color(200, 100, 100) : sf::Color(100, 200, 100);
    snprintf(buf, sizeof(buf), "Combat: %s", combat_str);
    rend.draw_text(buf, x + 8, y, combat_color, 10);

    // Safe attack mode (on same line)
    const char* safe_str = safe_attack_mode_ ? "Safe" : "PK";
    sf::Color safe_color = safe_attack_mode_ ? sf::Color(100, 200, 100) : sf::Color(200, 100, 100);
    snprintf(buf, sizeof(buf), "  Mode: %s", safe_str);
    rend.draw_text(buf, x + 100, y, safe_color, 10);
}

void debug_stats::set_camera_bounds(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    camera_left_ = left;
    camera_top_ = top;
    camera_right_ = right;
    camera_bottom_ = bottom;
}

void debug_stats::set_player_position(int32_t tile_x, int32_t tile_y, int32_t world_x, int32_t world_y)
{
    player_tile_x_ = tile_x;
    player_tile_y_ = tile_y;
    player_world_x_ = world_x;
    player_world_y_ = world_y;
}

void debug_stats::set_combat_mode(bool attack_mode, bool safe_mode)
{
    combat_mode_ = attack_mode;
    safe_attack_mode_ = safe_mode;
}

} // namespace debug
} // namespace hb
