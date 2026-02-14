#include "debug/debug_stats.hpp"
#include "entity/entity.hpp"
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

    // Update network message rate from total counters
    network_stats_timer_ += delta_time;
    if (network_stats_timer_ >= 1.0f)
    {
        messages_received_per_sec_ = static_cast<int32_t>(net_total_received_ - net_prev_received_);
        messages_sent_per_sec_ = static_cast<int32_t>(net_total_sent_ - net_prev_sent_);
        net_prev_received_ = net_total_received_;
        net_prev_sent_ = net_total_sent_;
        network_stats_timer_ = 0.0f;
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

    // Calculate dynamic box height by counting content lines
    int32_t content_lines = 0;
    int32_t spacings = 0;

    // Title + separator
    content_lines += 1;
    spacings += 1;

    // Performance: header + FPS + frame time + draw calls
    content_lines += 4;
    spacings += 1;

    // Network: header + status + ping + messages
    content_lines += 4;
    spacings += 1;

    // Rendering: header + entities + objects + tiles + chunks
    content_lines += 5;
    spacings += 1;

    // Assets: header + sprite cache + PAK files
    content_lines += 3;
    spacings += 1;

    // World: header + camera + entities + zoom + weather + time
    content_lines += 4;
    if (!map_name_.empty()) content_lines += 1;
    if (zoom_level_ != 1.0f) content_lines += 1;
    if (!weather_.empty()) content_lines += 1;
    if (!time_of_day_.empty()) content_lines += 1;
    spacings += 1;

    // Player: header + tile + world + movement + optional stats
    content_lines += 4;
    if (player_level_ > 0) content_lines += 2;
    spacings += 1;

    // Input: header + screen + world + tile + optional hover
    content_lines += 4;
    if (!hovered_entity_.empty()) content_lines += 1;
    spacings += 1;

    // Audio: header + BGM + sounds
    content_lines += 3;
    spacings += 1;

    // UI: header + dialogs
    content_lines += 2;
    spacings += 1;

    // Game State: header + optional state + combat
    content_lines += 2;
    if (!game_state_.empty()) content_lines += 1;

    int32_t box_height = padding_ + 6 + content_lines * line_height_ + spacings * section_spacing_ + 2 + padding_;

    // Draw semi-transparent background with soft shadow effect
    rend.draw_rect(padding_ + 3, padding_ + 3, box_width_, box_height,
                   sf::Color(0, 0, 0, 80), true);
    rend.draw_rect(padding_, padding_, box_width_, box_height,
                   sf::Color(20, 20, 30, 220), true);
    rend.draw_rect(padding_, padding_, box_width_, box_height,
                   sf::Color(60, 60, 80, 220), false);

    int32_t x = padding_ + 8;
    int32_t y = padding_ + 6;
    char buf[128];

    // Title
    rend.draw_text("Debug Stats", x, y, sf::Color(200, 200, 220), 11);
    rend.draw_text("(Settings)", x + 80, y, sf::Color(120, 120, 140), 9);
    y += line_height_ + 2;

    // Separator
    rend.draw_line(x, y, x + box_width_ - 16, y, sf::Color(60, 60, 80));
    y += section_spacing_;

    // === Performance ===
    render_section(rend, y, "Performance");

    snprintf(buf, sizeof(buf), "FPS: %.1f", current_fps_);
    sf::Color fps_color = current_fps_ >= 55.0f ? sf::Color(100, 200, 100) :
                          current_fps_ >= 30.0f ? sf::Color(200, 200, 100) :
                                                  sf::Color(200, 100, 100);
    rend.draw_text(buf, x + 8, y, fps_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Frame: %.2f ms", delta_time_ms_);
    sf::Color dt_color = delta_time_ms_ <= 17.0f ? sf::Color(100, 200, 100) :
                         delta_time_ms_ <= 33.0f ? sf::Color(200, 200, 100) :
                                                   sf::Color(200, 100, 100);
    rend.draw_text(buf, x + 8, y, dt_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Draw calls: %u", draw_calls_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === Network ===
    render_section(rend, y, "Network");

    const char* conn_status = network_connected_ ? "Connected" : "Disconnected";
    sf::Color conn_color = network_connected_ ? sf::Color(100, 200, 100) : sf::Color(200, 100, 100);
    snprintf(buf, sizeof(buf), "Status: %s", conn_status);
    rend.draw_text(buf, x + 8, y, conn_color, 10);
    y += line_height_;

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

    snprintf(buf, sizeof(buf), "Recv: %d/s  Sent: %d/s", messages_received_per_sec_, messages_sent_per_sec_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === Rendering ===
    render_section(rend, y, "Rendering");

    snprintf(buf, sizeof(buf), "Entities: %d", sprites_rendered_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Objects: %d", objects_rendered_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Tiles: %d", tiles_rendered_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Chunks: %d", chunk_count_);
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

    if (!map_name_.empty())
    {
        snprintf(buf, sizeof(buf), "Map: %s", map_name_.c_str());
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
        y += line_height_;
    }

    snprintf(buf, sizeof(buf), "Camera: (%d,%d)-(%d,%d)",
             camera_left_, camera_top_, camera_right_, camera_bottom_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Entities: %d", entity_count_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    if (zoom_level_ != 1.0f)
    {
        snprintf(buf, sizeof(buf), "Zoom: %.2fx", zoom_level_);
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
        y += line_height_;
    }

    if (!weather_.empty())
    {
        snprintf(buf, sizeof(buf), "Weather: %s", weather_.c_str());
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
        y += line_height_;
    }

    if (!time_of_day_.empty())
    {
        snprintf(buf, sizeof(buf), "Time: %s", time_of_day_.c_str());
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
        y += line_height_;
    }

    y += section_spacing_;

    // === Player ===
    render_section(rend, y, "Player");

    snprintf(buf, sizeof(buf), "Tile: (%d, %d)", player_tile_x_, player_tile_y_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "World: (%d, %d)", player_world_x_, player_world_y_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    // Movement state
    if (player_moving_)
    {
        const char* move_type = player_running_ ? "Running" : "Walking";
        snprintf(buf, sizeof(buf), "Move: %s %s (%.0f%%)",
                 move_type, player_direction_.c_str(), player_move_progress_ * 100.0f);
        rend.draw_text(buf, x + 8, y, sf::Color(200, 200, 100), 10);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Move: Idle (%s)", player_direction_.c_str());
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    }
    y += line_height_;

    if (player_level_ > 0)
    {
        snprintf(buf, sizeof(buf), "Level: %d", player_level_);
        rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "HP: %d/%d  MP: %d/%d",
                 player_hp_, player_max_hp_, player_mp_, player_max_mp_);
        sf::Color hp_color = (player_max_hp_ > 0 && player_hp_ * 100 / player_max_hp_ <= 25)
            ? sf::Color(200, 100, 100) : sf::Color(180, 180, 200);
        rend.draw_text(buf, x + 8, y, hp_color, 10);
        y += line_height_;
    }

    y += section_spacing_;

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

    // === Audio ===
    render_section(rend, y, "Audio");

    if (!bgm_track_.empty())
    {
        snprintf(buf, sizeof(buf), "BGM: %s", bgm_track_.c_str());
    }
    else
    {
        snprintf(buf, sizeof(buf), "BGM: (none)");
    }
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Active sounds: %d", active_sounds_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

    // === UI ===
    render_section(rend, y, "UI");

    snprintf(buf, sizeof(buf), "Open dialogs: %d", open_dialog_count_);
    rend.draw_text(buf, x + 8, y, sf::Color(180, 180, 200), 10);
    y += line_height_ + section_spacing_;

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

void debug_stats::set_player_stats(int32_t hp, int32_t max_hp, int32_t mp, int32_t max_mp, uint16_t level)
{
    player_hp_ = hp;
    player_max_hp_ = max_hp;
    player_mp_ = mp;
    player_max_mp_ = max_mp;
    player_level_ = level;
}

void debug_stats::set_player_movement(const std::string& direction, bool moving, float progress, bool running)
{
    player_direction_ = direction;
    player_moving_ = moving;
    player_move_progress_ = progress;
    player_running_ = running;
}

void debug_stats::render_entity_info(renderer& rend, int32_t screen_width, int32_t screen_height)
{
    if (!entity_info_visible_)
    {
        return;
    }

    const entity* ent = pinned_entity_ ? pinned_entity_ : hovered_entity_ptr_;
    if (!ent)
    {
        return;
    }

    // Colors
    const auto header_color = sf::Color(140, 180, 220);
    const auto data_color = sf::Color(180, 180, 200);
    const auto name_color = sf::Color(200, 200, 100);
    const auto moving_color = sf::Color(200, 200, 100);
    const auto low_hp_color = sf::Color(200, 100, 100);
    const auto status_color = sf::Color(200, 100, 200);

    // Count lines dynamically
    int32_t content_lines = 0;
    int32_t spacings = 0;

    // Title + separator
    content_lines += 1;
    spacings += 1;

    // Identity: header + ID + type + visual_type + alive + current_action
    content_lines += 6;
    spacings += 1;

    // Transform: header + tile/move_start + world + dir/moving/progress
    content_lines += 4;
    spacings += 1;

    // Animation: header + state/frame + timer/duration + loop/finished
    content_lines += 4;
    spacings += 1;

    // Name (optional)
    if (ent->has_name())
    {
        content_lines += 2; // header + name
        if (!ent->name().guild_name.empty())
        {
            content_lines += 1;
        }
        content_lines += 1; // faction/hostile/pk/gm
        spacings += 1;
    }

    // Stats (optional)
    if (ent->has_stats())
    {
        content_lines += 6; // header + level/exp + HP/MP/SP + base stats + combat stats (2 lines)
        spacings += 1;
    }

    // Combat (optional)
    if (ent->has_combat())
    {
        content_lines += 3; // header + mode/target/attack + status effects
        spacings += 1;
    }

    // Movement (optional)
    if (ent->has_movement())
    {
        content_lines += 2; // header + speed/run/running/can_move
        spacings += 1;
    }

    // Monster (optional)
    if (ent->has_monster())
    {
        content_lines += 3; // header + type/level/owner + boss/summon/aggro/category
        spacings += 1;
    }

    // NPC (optional)
    if (ent->has_npc())
    {
        content_lines += 3; // header + type/id + roles
        spacings += 1;
    }

    // Sprite: header + gender/skin/hair/underwear
    content_lines += 2;

    int32_t box_height = padding_ + 6 + content_lines * line_height_ + spacings * section_spacing_ + 2 + padding_;

    // Position at bottom-right, above icon panel
    int32_t box_x = screen_width - entity_info_width_ - 10;
    int32_t box_y = screen_height - icon_panel_height_ - box_height - 10;

    // Draw background (shadow + fill + border)
    rend.draw_rect(box_x + 3, box_y + 3, entity_info_width_, box_height,
                   sf::Color(0, 0, 0, 80), true);
    rend.draw_rect(box_x, box_y, entity_info_width_, box_height,
                   sf::Color(20, 20, 30, 220), true);
    rend.draw_rect(box_x, box_y, entity_info_width_, box_height,
                   sf::Color(60, 60, 80, 220), false);

    int32_t x = box_x + 8;
    int32_t y = box_y + 6;
    char buf[256];

    // Title
    if (pinned_entity_)
    {
        rend.draw_text("Entity Info (pinned)", x, y, sf::Color(200, 200, 220), 11);
    }
    else
    {
        rend.draw_text("Entity Info", x, y, sf::Color(200, 200, 220), 11);
    }
    y += line_height_ + 2;

    // Separator
    rend.draw_line(x, y, x + entity_info_width_ - 16, y, sf::Color(60, 60, 80));
    y += section_spacing_;

    // === Identity ===
    rend.draw_text("Identity", x, y, header_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "ID: %u", ent->id());
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Type: %d", static_cast<int>(ent->type()));
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Visual: %u", ent->visual_type());
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Alive: %s", ent->is_alive() ? "yes" : "no");
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Action: %d", static_cast<int>(ent->current_action()));
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_ + section_spacing_;

    // === Transform ===
    const auto& tf = ent->transform();

    rend.draw_text("Transform", x, y, header_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Tile: (%d,%d)  Start: (%d,%d)",
             tf.tile_x, tf.tile_y, tf.move_start_x, tf.move_start_y);
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "World: (%d,%d)", tf.x, tf.y);
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Dir: %d  Moving: %s  Prog: %.0f%%",
             static_cast<int>(tf.facing), tf.moving ? "yes" : "no",
             tf.move_progress * 100.0f);
    sf::Color move_line_color = tf.moving ? moving_color : data_color;
    rend.draw_text(buf, x + 8, y, move_line_color, 10);
    y += line_height_ + section_spacing_;

    // === Animation ===
    const auto& anim = ent->animation();

    rend.draw_text("Animation", x, y, header_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "State: %d  Frame: %d/%d",
             static_cast<int>(anim.state), anim.current_frame, anim.frame_count);
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Timer: %.0fms  Dur: %.0fms",
             anim.frame_timer * 1000.0f, anim.frame_duration * 1000.0f);
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Loop: %s  Finished: %s",
             anim.looping ? "yes" : "no", anim.finished ? "yes" : "no");
    rend.draw_text(buf, x + 8, y, data_color, 10);
    y += line_height_ + section_spacing_;

    // === Name (optional) ===
    if (ent->has_name())
    {
        const auto& nm = ent->name();

        rend.draw_text("Name", x, y, header_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "Name: %s", nm.name.c_str());
        rend.draw_text(buf, x + 8, y, name_color, 10);
        y += line_height_;

        if (!nm.guild_name.empty())
        {
            snprintf(buf, sizeof(buf), "Guild: %s", nm.guild_name.c_str());
            rend.draw_text(buf, x + 8, y, data_color, 10);
            y += line_height_;
        }

        const char* hostile_str = "neutral";
        if (nm.hostile == hostility::friendly) hostile_str = "friendly";
        else if (nm.hostile == hostility::enemy) hostile_str = "enemy";

        const char* pk_str = "innocent";
        if (nm.pk == pk_status::criminal) pk_str = "criminal";
        else if (nm.pk == pk_status::murderer) pk_str = "murderer";

        snprintf(buf, sizeof(buf), "%s  %s  %s%s",
                 nm.faction.empty() ? "no-faction" : nm.faction.c_str(),
                 hostile_str, pk_str,
                 nm.is_gm ? "  GM" : "");
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_ + section_spacing_;
    }

    // === Stats (optional) ===
    if (ent->has_stats())
    {
        const auto& st = ent->stats();

        rend.draw_text("Stats", x, y, header_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "Lv: %u  Exp: %u", st.level, st.exp);
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_;

        bool hp_low = st.max_hp > 0 && st.hp * 100 / st.max_hp <= 25;
        snprintf(buf, sizeof(buf), "HP: %d/%d  MP: %d/%d  SP: %d/%d",
                 st.hp, st.max_hp, st.mp, st.max_mp, st.sp, st.max_sp);
        rend.draw_text(buf, x + 8, y, hp_low ? low_hp_color : data_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "STR:%u VIT:%u DEX:%u INT:%u MAG:%u CHA:%u",
                 st.strength, st.vitality, st.dexterity,
                 st.intelligence, st.magic, st.charisma);
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "ATK:%d DEF:%d HIT:%d DOD:%d",
                 st.attack_power, st.defense, st.hit_ratio, st.dodge_ratio);
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_ + section_spacing_;
    }

    // === Combat (optional) ===
    if (ent->has_combat())
    {
        const auto& cb = ent->combat();

        rend.draw_text("Combat", x, y, header_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "Mode: %d  Target: %u  AtkType: %d",
                 static_cast<int>(cb.mode), cb.target_id, cb.attack_type);
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_;

        bool any_status = cb.poisoned || cb.paralyzed || cb.frozen
                       || cb.invisible || cb.invulnerable;
        snprintf(buf, sizeof(buf), "PSN:%d PAR:%d FRZ:%d INV:%d INVULN:%d",
                 cb.poisoned, cb.paralyzed, cb.frozen,
                 cb.invisible, cb.invulnerable);
        rend.draw_text(buf, x + 8, y, any_status ? status_color : data_color, 10);
        y += line_height_ + section_spacing_;
    }

    // === Movement (optional) ===
    if (ent->has_movement())
    {
        const auto& mv = ent->movement();

        rend.draw_text("Movement", x, y, header_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "Spd: %.1f  Run: %.1f  Running: %s  CanMove: %s",
                 mv.speed, mv.run_speed,
                 mv.running ? "yes" : "no",
                 mv.can_move ? "yes" : "no");
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_ + section_spacing_;
    }

    // === Monster (optional) ===
    if (ent->has_monster())
    {
        const auto& mon = ent->monster();

        rend.draw_text("Monster", x, y, header_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "Type: %u  Lv: %u  Owner: %u",
                 mon.monster_type, mon.monster_level, mon.owner_id);
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "Boss:%d Sum:%d Aggro:%d  %s",
                 mon.is_boss, mon.is_summon, mon.is_aggressive,
                 mon.category.empty() ? "" : mon.category.c_str());
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_ + section_spacing_;
    }

    // === NPC (optional) ===
    if (ent->has_npc())
    {
        const auto& np = ent->npc();

        rend.draw_text("NPC", x, y, header_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "Type: %u  ID: %u", np.npc_type, np.npc_id);
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_;

        snprintf(buf, sizeof(buf), "%s%s%s%s%s",
                 np.is_shop ? "Shop " : "",
                 np.is_quest_giver ? "Quest " : "",
                 np.is_banker ? "Bank " : "",
                 np.is_warehouse ? "WH " : "",
                 np.is_skill_trainer ? "Train" : "");
        rend.draw_text(buf, x + 8, y, data_color, 10);
        y += line_height_ + section_spacing_;
    }

    // === Sprite ===
    const auto& sp = ent->sprite();

    rend.draw_text("Sprite", x, y, header_color, 10);
    y += line_height_;

    snprintf(buf, sizeof(buf), "Gender:%d Skin:%d Hair:%d/%d Undw:%d",
             sp.gender, sp.skin_color, sp.hair_style, sp.hair_color,
             sp.underwear_color);
    rend.draw_text(buf, x + 8, y, data_color, 10);
}

} // namespace debug
} // namespace hb
