#include "gameplay/input_handler.hpp"
#include "gameplay/game_state.hpp"
#include "input/input.hpp"
#include "core/direction_utils.hpp"
#include "gameplay/pathfinding.hpp"
#include "ui/cursor.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void input_handler::initialize(game_state_manager& game)
{
    game_ = &game;
}

void input_handler::clear()
{
    move_dest_x_ = -1;
    move_dest_y_ = -1;
    player_turn_ = false;
    prev_tile_x_ = -1;
    prev_tile_y_ = -1;
    mouse_x_ = 0;
    mouse_y_ = 0;
    combat_mode_ = false;
    safe_attack_mode_ = false;
    force_attack_mode_ = false;
    run_mode_enabled_ = false;
    suppress_until_release_ = false;
    camera_drag_locked_ = false;
    spell_targeting_active_ = false;
    attack_consumed_ = false;
    blocked_movement_cooldown_ = 0.0f;
}

void input_handler::update(float /*delta_time*/) {}

bool input_handler::can_perform_action() const
{
    const entity* player = game_->local_player();
    if (!player)
        return false;

    if (!player->is_alive())
        return false;

    if (blocked_movement_cooldown_ > 0.0f)
        return false;

    if (player->transform().moving)
        return false;

    const auto& anim = player->animation();
    if (!anim.looping && !anim.finished)
        return false;

    return true;
}

void input_handler::update_cooldown(float delta_time)
{
    if (blocked_movement_cooldown_ > 0.0f)
    {
        blocked_movement_cooldown_ -= delta_time;
        if (blocked_movement_cooldown_ < 0.0f)
            blocked_movement_cooldown_ = 0.0f;
    }
}

void input_handler::set_move_dest(int32_t x, int32_t y)
{
    move_dest_x_ = x;
    move_dest_y_ = y;
    if (x >= 0 && y >= 0)
    {
        prev_tile_x_ = -1;
        prev_tile_y_ = -1;
    }
}

void input_handler::toggle_combat_mode()
{
    combat_mode_ = !combat_mode_;
}

void input_handler::handle_input(const input& inp)
{
    if (suppress_until_release_)
    {
        if (!inp.is_mouse_down(sf::Mouse::Button::Left) && !inp.is_mouse_down(sf::Mouse::Button::Right))
        {
            suppress_until_release_ = false;
        }
        return;
    }

    attack_consumed_ = false;
    handle_playing_input(inp);
}

void input_handler::handle_playing_input(const input& inp)
{
    auto& ui = game_->ui();

    // Update UI mouse consumed tracking
    ui.update_mouse_consumed(inp);

    if (ui.is_modal_open())
    {
        return;
    }

    if (ui.is_mouse_consumed(sf::Mouse::Button::Left) || ui.is_mouse_consumed(sf::Mouse::Button::Right))
    {
        handle_hotkey_input(inp);
        return;
    }

    // Check if we should enter/handle spell targeting mode
    auto& magic = game_->magic();
    if (magic.has_pending_spell() && !spell_targeting_active_)
    {
        spell_targeting_active_ = true;

        // Auto-switch to attack mode when readying a spell
        if (!combat_mode_)
        {
            game_->toggle_combat_mode();
            spdlog::debug("Combat mode auto-enabled for spell targeting");
        }

        // Play casting animation on local player (sound triggered by entity_manager at frame 1)
        entity* player = game_->local_player();
        if (player)
        {
            player->set_action(object_action::magic);

            // Show spell name above player's head as red chat bubble
            const auto* sp = magic.get_spell(magic.pending_spell());
            if (sp && player->has_name())
            {
                auto& name = player->name();
                name.chat_message = sp->name + "!";
                name.chat_timer = 3.0f;
                name.chat_elapsed = 0.0f;
                name.chat_style = {sf::Color::Red, sf::Color::Black, 1.0f, 14};
            }
        }

        spdlog::debug("Spell targeting mode activated for spell {}", magic.pending_spell());
    }

    if (spell_targeting_active_)
    {
        handle_spell_targeting(inp);
        handle_hotkey_input(inp);
        return;
    }

    auto& world = game_->game_world();

    // Ctrl+click drag to pan in cinematic mode
    if (world.is_cinematic_mode() && !camera_drag_locked_)
    {
        bool ctrl_held = inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl);

        if (ctrl_held && inp.is_mouse_pressed(sf::Mouse::Button::Left))
        {
            world.start_drag(inp.mouse_x(), inp.mouse_y());
        }
        else if (world.is_dragging())
        {
            if (inp.is_mouse_down(sf::Mouse::Button::Left) && ctrl_held)
            {
                world.update_drag(inp.mouse_x(), inp.mouse_y());
            }
            else
            {
                world.end_drag();
            }
        }

        if (world.is_dragging())
        {
            handle_hotkey_input(inp);
            return;
        }
    }

    // Block combat and movement while dead — only hotkeys (chat) allowed
    entity* local = game_->local_player();
    if (local && !local->is_alive())
    {
        handle_hotkey_input(inp);
        return;
    }

    // Right-click anywhere on the game world clears the walk destination
    if (inp.is_mouse_pressed(sf::Mouse::Button::Right) && move_dest_x_ >= 0)
    {
        move_dest_x_ = -1;
        move_dest_y_ = -1;
        if (local && local->has_movement())
        {
            local->movement().target_x = -1;
            local->movement().target_y = -1;
        }
    }

    handle_combat_input(inp);
    if (!attack_consumed_)
    {
        handle_movement_input(inp);
    }
    update_pathfinding_trace();
    handle_hotkey_input(inp);
}

void input_handler::handle_movement_input(const input& inp)
{
    entity* player = game_->local_player();
    if (!player || !player->has_movement())
    {
        return;
    }

    // Don't process movement while in a non-interruptible animation
    {
        auto state = player->animation().state;
        if (!player->animation().finished &&
            (state == entity_anim_state::attack || state == entity_anim_state::attack_move ||
             state == entity_anim_state::damage || state == entity_anim_state::magic ||
             state == entity_anim_state::magic_attack))
        {
            return;
        }
    }

    auto& world = game_->game_world();
    auto& entities = game_->entities();
    auto& sprites = game_->sprites();

    if (inp.is_mouse_pressed(sf::Mouse::Button::Left))
    {
        [[maybe_unused]] auto [dest_x, dest_y] = world.screen_to_tile(mouse_x_, mouse_y_);
        auto& t = player->transform();

        bool clicked_on_self = entities.is_point_in_entity_sprite(
            *player, sprites, world.camera_x(), world.camera_y(), mouse_x_, mouse_y_);

        if (clicked_on_self)
        {
            bool ctrl_held =
                inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl);

            if (ctrl_held)
            {
                int32_t north_x = t.tile_x;
                int32_t north_y = t.tile_y - 1;
                entity* target = entities.find_at_tile(north_x, north_y);

                if (target && target->type() == entity_type::monster)
                {
                    uint8_t atk_type = 0;
                    if (const auto* weapon = game_->inventory().get_equipped(equip_slot::right_hand))
                    {
                        if (is_bow_weapon(weapon->type_id))
                            atk_type = static_cast<uint8_t>(attack_type::ranged);
                    }

                    if (can_perform_action())
                    {
                        spdlog::debug("Ctrl+click on self: attacking north at ({},{})", north_x, north_y);
                        game_->ws_handler().request_attack(target->id(), atk_type);
                        player->transform().facing = direction::north;
                    }
                }
                else
                {
                    spdlog::debug("Ctrl+click on self: no enemy to north");
                }
            }
            else
            {
                if (can_perform_action())
                {
                    spdlog::debug("Click on self: sending pickup request at ({},{})", t.tile_x, t.tile_y);
                    game_->request_pickup(t.tile_x, t.tile_y);
                }
            }
            return;
        }
    }

    if (inp.is_mouse_down(sf::Mouse::Button::Left))
    {
        // During a one-shot animation (pickup, attack), don't re-evaluate the
        // mouse — sprite bounds shift mid-animation and would cause false
        // movement targets. Wait until the animation finishes to decide.
        const auto& anim = player->animation();
        if (!anim.looping && !anim.finished)
        {
            return;
        }

        bool hovering_self = entities.is_point_in_entity_sprite(
            *player, sprites, world.camera_x(), world.camera_y(), mouse_x_, mouse_y_);
        if (hovering_self)
        {
            auto& t = player->transform();
            if (can_perform_action())
            {
                game_->request_pickup(t.tile_x, t.tile_y);
            }
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            return;
        }

        auto [dest_x, dest_y] = world.screen_to_tile(mouse_x_, mouse_y_);
        auto& t = player->transform();
        if (dest_x == t.tile_x && dest_y == t.tile_y)
        {
            return;
        }

        if (dest_x != move_dest_x_ || dest_y != move_dest_y_)
        {
            move_dest_x_ = dest_x;
            move_dest_y_ = dest_y;
            prev_tile_x_ = -1;
            prev_tile_y_ = -1;
        }
    }

    if (inp.is_mouse_down(sf::Mouse::Button::Right))
    {
        auto& t = player->transform();

        auto [click_x, click_y] = world.screen_to_tile(mouse_x_, mouse_y_);
        int32_t dx = click_x - t.tile_x;
        int32_t dy = click_y - t.tile_y;

        std::optional<direction> face_dir;
        if (dx == 0 && dy == 0)
        {
            if (t.moving)
            {
                face_dir = t.facing;
            }
        }
        else if (std::abs(dx) > std::abs(dy) * 2)
        {
            face_dir = (dx > 0) ? direction::east : direction::west;
        }
        else if (std::abs(dy) > std::abs(dx) * 2)
        {
            face_dir = (dy > 0) ? direction::south : direction::north;
        }
        else
        {
            if (dx > 0 && dy < 0)
                face_dir = direction::north_east;
            else if (dx > 0 && dy > 0)
                face_dir = direction::south_east;
            else if (dx < 0 && dy > 0)
                face_dir = direction::south_west;
            else if (dx < 0 && dy < 0)
                face_dir = direction::north_west;
        }

        if (face_dir && (t.moving || t.facing != *face_dir))
        {
            t.facing = *face_dir;

            if (can_perform_action())
            {
                json msg = make_player_stop_request(
                    t.tile_x, t.tile_y, static_cast<uint8_t>(direction_to_protocol(*face_dir)));
                game_->ws_connection().send(msg);
            }
        }
    }

    // Right click held interrupts pathfinding
    if (inp.is_mouse_down(sf::Mouse::Button::Right) && move_dest_x_ >= 0)
    {
        move_dest_x_ = -1;
        move_dest_y_ = -1;
        if (player->has_movement())
        {
            player->movement().target_x = -1;
            player->movement().target_y = -1;
        }
        if (!player->transform().moving)
        {
            player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
        }
    }

    // Continue moving toward destination (pathfinding)
    if (move_dest_x_ >= 0 && move_dest_y_ >= 0 && !player->transform().moving && can_perform_action())
    {
        auto& t = player->transform();

        auto is_passable = [&](int32_t x, int32_t y) -> bool
        {
            if (!world.current_map().is_walkable(x, y))
                return false;
            for (auto* e : entities.get_entities_on_tile(x, y))
            {
                if (e && e->is_alive() && e != player)
                    return false;
            }
            return true;
        };

        // If we're adjacent to (or on) the destination and it's blocked, stop.
        // Pathfinding should route around obstacles EN ROUTE, not around the destination itself.
        int32_t dest_dx = std::abs(t.tile_x - move_dest_x_);
        int32_t dest_dy = std::abs(t.tile_y - move_dest_y_);
        if (dest_dx <= 1 && dest_dy <= 1 && !is_passable(move_dest_x_, move_dest_y_))
        {
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
            return;
        }

        auto dir = get_next_walkable_dir(t.tile_x, t.tile_y, move_dest_x_, move_dest_y_, player_turn_, is_passable);

        if (!dir)
        {
            move_dest_x_ = -1;
            move_dest_y_ = -1;
            player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
            return;
        }

        player_turn_ = !player_turn_;

        auto [ddx, ddy] = direction_offset(*dir);
        int32_t next_x = t.tile_x + ddx;
        int32_t next_y = t.tile_y + ddy;

        if (next_x == prev_tile_x_ && next_y == prev_tile_y_)
        {
            bool actively_moving = inp.is_mouse_down(sf::Mouse::Button::Left) ||
                                   inp.is_key_down(sf::Keyboard::Key::W) || inp.is_key_down(sf::Keyboard::Key::A) ||
                                   inp.is_key_down(sf::Keyboard::Key::S) || inp.is_key_down(sf::Keyboard::Key::D) ||
                                   inp.is_key_down(sf::Keyboard::Key::Up) || inp.is_key_down(sf::Keyboard::Key::Down) ||
                                   inp.is_key_down(sf::Keyboard::Key::Left) ||
                                   inp.is_key_down(sf::Keyboard::Key::Right);
            if (!actively_moving)
            {
                move_dest_x_ = -1;
                move_dest_y_ = -1;
                player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
                return;
            }
        }

        prev_tile_x_ = t.tile_x;
        prev_tile_y_ = t.tile_y;

        uint8_t dir_protocol = static_cast<uint8_t>(direction_to_protocol(*dir));

        bool should_run = inp.is_key_down(sf::Keyboard::Key::LShift) || inp.is_key_down(sf::Keyboard::Key::RShift) ||
                          run_mode_enabled_;

        // Save origin, then immediately set tile to destination
        t.move_start_x = t.tile_x;
        t.move_start_y = t.tile_y;
        t.tile_x = next_x;
        t.tile_y = next_y;
        t.facing = *dir;
        t.moving = true;
        t.move_progress = 0.0f;

        if (player->has_movement())
        {
            player->movement().running = should_run;
            player->movement().target_x = move_dest_x_;
            player->movement().target_y = move_dest_y_;
        }

        object_action base_action = should_run ? object_action::run : object_action::move_peace;
        player->set_action_with_combat_mode(base_action, combat_mode_);

        // Send origin position (move_start) as source in network message
        json msg = make_player_move_request(
            t.move_start_x, t.move_start_y, dir_protocol, should_run, move_dest_x_, move_dest_y_);
        game_->ws_connection().send(msg);
    }

    // Keyboard movement
    int32_t dx = 0, dy = 0;
    if (inp.is_key_down(sf::Keyboard::Key::W) || inp.is_key_down(sf::Keyboard::Key::Up))
        dy = -1;
    if (inp.is_key_down(sf::Keyboard::Key::S) || inp.is_key_down(sf::Keyboard::Key::Down))
        dy = 1;
    if (inp.is_key_down(sf::Keyboard::Key::A) || inp.is_key_down(sf::Keyboard::Key::Left))
        dx = -1;
    if (inp.is_key_down(sf::Keyboard::Key::D) || inp.is_key_down(sf::Keyboard::Key::Right))
        dx = 1;

    if (dx != 0 || dy != 0)
    {
        move_dest_x_ = -1;
        move_dest_y_ = -1;
    }

    if ((dx != 0 || dy != 0) && !player->transform().moving && can_perform_action())
    {
        auto& t = player->transform();
        auto& world_ref = game_->game_world();
        auto& entities_ref = game_->entities();
        int32_t next_x = t.tile_x + dx;
        int32_t next_y = t.tile_y + dy;

        auto move_dir = calculate_direction(t.tile_x, t.tile_y, next_x, next_y);
        if (!move_dir)
        {
            return;
        }

        if (!world_ref.current_map().is_walkable(next_x, next_y))
        {
            return;
        }

        auto entities_on_tile = entities_ref.get_entities_on_tile(next_x, next_y);
        for (auto* e : entities_on_tile)
        {
            if (e && e->is_alive() && e != player)
            {
                return;
            }
        }

        bool should_run = inp.is_key_down(sf::Keyboard::Key::LShift) || inp.is_key_down(sf::Keyboard::Key::RShift) ||
                          run_mode_enabled_;

        // Save origin, then immediately set tile to destination
        t.move_start_x = t.tile_x;
        t.move_start_y = t.tile_y;
        t.tile_x = next_x;
        t.tile_y = next_y;
        t.facing = *move_dir;
        t.moving = true;
        t.move_progress = 0.0f;

        if (player->has_movement())
        {
            player->movement().running = should_run;
            player->movement().target_x = next_x;
            player->movement().target_y = next_y;
        }

        object_action base_action = should_run ? object_action::run : object_action::move_peace;
        player->set_action_with_combat_mode(base_action, combat_mode_);

        uint8_t dir_protocol = static_cast<uint8_t>(direction_to_protocol(*move_dir));
        json msg = make_player_move_request(t.move_start_x, t.move_start_y, dir_protocol, should_run, next_x, next_y);
        game_->ws_connection().send(msg);
    }

    // Deferred idle: when the movement system skips the idle transition at tile
    // arrival (to avoid a 1-frame animation flash between tiles), we handle the
    // transition here. If no movement input is active, go to idle now.
    if (!player->transform().moving)
    {
        auto state = player->animation().state;
        if (state == entity_anim_state::run || state == entity_anim_state::move)
        {
            bool has_movement_input =
                move_dest_x_ >= 0 || inp.is_key_down(sf::Keyboard::Key::W) || inp.is_key_down(sf::Keyboard::Key::S) ||
                inp.is_key_down(sf::Keyboard::Key::A) || inp.is_key_down(sf::Keyboard::Key::D) ||
                inp.is_key_down(sf::Keyboard::Key::Up) || inp.is_key_down(sf::Keyboard::Key::Down) ||
                inp.is_key_down(sf::Keyboard::Key::Left) || inp.is_key_down(sf::Keyboard::Key::Right);

            if (!has_movement_input)
            {
                player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
            }
        }
    }
}

void input_handler::update_pathfinding_trace()
{
    auto& world = game_->game_world();

    if (!world.render_config().show_walkability)
    {
        world.clear_pathfinding_trace();
        return;
    }

    entity* player = game_->local_player();
    if (!player || move_dest_x_ < 0 || move_dest_y_ < 0)
    {
        world.clear_pathfinding_trace();
        return;
    }

    auto& t = player->transform();
    int32_t cur_x = t.tile_x;
    int32_t cur_y = t.tile_y;

    const auto& m = world.current_map();
    std::vector<std::pair<int32_t, int32_t>> trace;

    if (t.moving)
    {
        // tile_x/y is already the destination during movement
        trace.emplace_back(cur_x, cur_y);
    }
    bool turn = player_turn_;

    static constexpr int32_t max_trace_steps = 200;

    for (int32_t step = 0; step < max_trace_steps; ++step)
    {
        if (cur_x == move_dest_x_ && cur_y == move_dest_y_)
            break;

        auto dir = get_next_walkable_dir(
            cur_x, cur_y, move_dest_x_, move_dest_y_, turn, [&](int32_t x, int32_t y) { return m.is_walkable(x, y); });

        if (!dir)
            break;

        turn = !turn;

        auto [ddx, ddy] = direction_offset(*dir);
        int32_t next_x = cur_x + ddx;
        int32_t next_y = cur_y + ddy;

        if (!trace.empty() && next_x == trace.back().first && next_y == trace.back().second)
            break;

        trace.emplace_back(next_x, next_y);
        cur_x = next_x;
        cur_y = next_y;
    }

    world.set_pathfinding_trace(std::move(trace));
}

void input_handler::handle_spell_targeting(const input& inp)
{
    auto& magic = game_->magic();
    auto& world = game_->game_world();
    auto& entities = game_->entities();

    // Set magic targeting cursor
    if (auto* cursor = game_->get_cursor())
    {
        // Check if hovering over an enemy entity for the arrow cursor variant
        entity* hover = entities.get_entity_at_screen_pos(mouse_x_, mouse_y_, world.camera_x(), world.camera_y());
        if (hover && hover->id() != entities.local_player_id() &&
            (hover->type() == entity_type::monster || hover->type() == entity_type::character))
            cursor->set_cursor(cursor_type::magic_arrow);
        else
            cursor->set_cursor(cursor_type::magic_target);
    }

    uint16_t pending_id = magic.pending_spell();
    const auto* sp = magic.get_spell(pending_id);
    if (!sp)
    {
        magic.clear_pending_spell();
        spell_targeting_active_ = false;
        return;
    }

    // Cancel targeting on right-click or Escape
    if (inp.is_mouse_pressed(sf::Mouse::Button::Right) || inp.is_key_pressed(sf::Keyboard::Key::Escape))
    {
        spdlog::debug("Spell targeting cancelled");
        magic.clear_pending_spell();
        spell_targeting_active_ = false;
        return;
    }

    // Execute targeting on left-click
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left))
    {
        entity* player = game_->local_player();
        if (!player)
        {
            magic.clear_pending_spell();
            spell_targeting_active_ = false;
            return;
        }

        const auto& player_t = player->transform();
        int32_t target_x = 0;
        int32_t target_y = 0;
        uint32_t target_id = 0;

        // Target the clicked tile; if an entity is under cursor, use their tile position
        {
            entity* target = entities.get_entity_at_screen_pos(mouse_x_, mouse_y_, world.camera_x(), world.camera_y());
            if (target && target->id() != entities.local_player_id())
            {
                target_id = target->id();
                target_x = target->transform().tile_x;
                target_y = target->transform().tile_y;
            }
            else
            {
                auto [tx, ty] = world.screen_to_tile(mouse_x_, mouse_y_);
                target_x = tx;
                target_y = ty;
            }
        }

        // Send cast request to server
        game_->ws_handler().request_magic(pending_id, target_x, target_y, target_id);

        // Trigger cooldown (the ready animation is already playing on the player)
        magic.trigger_cooldown(pending_id);

        // Spawn projectile and/or impact effects at tile center
        if (sp)
        {
            // Source: player's world position
            float src_wx = static_cast<float>(player_t.x);
            float src_wy = static_cast<float>(player_t.y);

            // Destination: target tile center (already computed above)
            float dest_wx = static_cast<float>(target_x * 32 + 16);
            float dest_wy = static_cast<float>(target_y * 32 + 16);

            if (sp->projectile_effect != 0)
            {
                game_->effects().add_effect_world(
                    static_cast<effect_type_id>(sp->projectile_effect), src_wx, src_wy, dest_wx, dest_wy);
            }
            else if (sp->effect_sprite != 0)
            {
                game_->effects().add_effect_at_pixel(static_cast<effect_type_id>(sp->effect_sprite), dest_wx, dest_wy);
            }
        }

        magic.clear_pending_spell();
        spell_targeting_active_ = false;

        // Suppress input until mouse is released to prevent the held click
        // from triggering movement after the cast
        suppress_until_release_ = true;
    }
}

void input_handler::handle_combat_input(const input& inp)
{
    auto& entities = game_->entities();
    auto& world = game_->game_world();

    bool left_click = inp.is_mouse_down(sf::Mouse::Button::Left);
    bool right_click = inp.is_mouse_down(sf::Mouse::Button::Right);
    if (left_click || right_click)
    {
        // Don't start a new attack while a non-interruptible animation is still playing
        entity* player = game_->local_player();
        if (player)
        {
            auto& anim = player->animation();
            if (!anim.finished &&
                (anim.state == entity_anim_state::attack || anim.state == entity_anim_state::attack_move ||
                 anim.state == entity_anim_state::magic || anim.state == entity_anim_state::magic_attack))
            {
                attack_consumed_ = true; // Still block movement while attacking
                return;
            }
        }

        entity* target = entities.get_entity_at_screen_pos(mouse_x_, mouse_y_, world.camera_x(), world.camera_y());

        // Only attack if the click landed on the entity's actual sprite, not just its tile
        if (target && !entities.is_point_in_entity_sprite(
                          *target, game_->sprites(), world.camera_x(), world.camera_y(), mouse_x_, mouse_y_))
        {
            target = nullptr;
        }

        if (target && target->id() != entities.local_player_id())
        {
            bool ctrl_held =
                inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl);

            // Corpses: walk on top unless Ctrl is held
            if (!target->is_alive() && !ctrl_held)
                return;

            // Determine hostility from name component
            auto target_hostility = hostility::neutral;
            if (target->has_name())
                target_hostility = target->name().hostile;

            // Hostile entities: attack on plain click
            // Friendly/neutral entities: Ctrl required
            if (target_hostility != hostility::enemy && !ctrl_held)
                return;

            // Determine attack type based on equipped weapon
            uint8_t atk_type = 0;
            if (const auto* weapon = game_->inventory().get_equipped(equip_slot::right_hand))
            {
                if (is_bow_weapon(weapon->type_id))
                {
                    atk_type = static_cast<uint8_t>(attack_type::ranged);
                }
            }

            // Don't attack if out of range
            bool ranged = atk_type == static_cast<uint8_t>(attack_type::ranged);
            auto& cs = game_->combat();
            entity_id pid = entities.local_player_id();
            if (ranged)
            {
                if (!cs.is_in_ranged_range(pid, target->id()))
                    return; // Out of range — right-click will face via movement handler
            }
            else if (!cs.is_in_melee_range(pid, target->id()))
            {
                // Not adjacent — right-click never walks/dashes, just return
                // (face-direction in handle_movement_input will run)
                if (right_click)
                    return;

                // Left-click: check for dash attack (2-tile range)
                bool ctrl_held_now =
                    inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl);
                if ((ctrl_held_now || force_attack_mode_) && cs.is_in_dash_range(pid, target->id()) &&
                    cs.can_dash_attack(pid))
                {
                    entity* player_ent = game_->local_player();
                    if (player_ent && !player_ent->transform().moving && can_perform_action())
                    {
                        execute_dash_attack(target, inp);
                        attack_consumed_ = true;
                    }
                }
                return;
            }

            attack_consumed_ = true;

            if (can_perform_action())
            {
                game_->ws_handler().request_attack(target->id(), atk_type);

                // Immediate local attack animation (don't wait for server round-trip)
                if (player)
                {
                    auto dir = calculate_direction(player->transform().tile_x,
                                                   player->transform().tile_y,
                                                   target->transform().tile_x,
                                                   target->transform().tile_y);
                    if (dir)
                        player->transform().facing = *dir;

                    if (atk_type == static_cast<uint8_t>(attack_type::ranged))
                        player->set_action(object_action::attack_combat_bow);
                    else
                        player->set_action_with_combat_mode(object_action::attack_peace, combat_mode_);
                }
            }
        }
    }

    for (int i = 0; i < 9; ++i)
    {
        sf::Keyboard::Key key = static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::Num1) + i);

        if (inp.is_key_pressed(key))
        {
            uint16_t hotbar_spell = game_->get_spell_hotbar_slot(static_cast<size_t>(i));
            if (hotbar_spell != 0 && game_->magic().has_spell(hotbar_spell))
            {
                game_->magic().set_pending_spell(hotbar_spell);
                spdlog::debug("Hotbar key {} -> spell {}", i + 1, hotbar_spell);
            }
        }
    }
}

void input_handler::handle_hotkey_input(const input& inp)
{
    auto& world = game_->game_world();
    auto& ui = game_->ui();
    auto& entities = game_->entities();
    auto& transition = game_->transition();
    auto& status_log = game_->get_status_log();
    auto& floating_text = game_->floating_text();
    auto* rend = game_->get_renderer();

    // Toggle cinematic mode (F5)
    if (inp.is_key_pressed(sf::Keyboard::Key::F5))
    {
        bool cinematic = !world.is_cinematic_mode();
        world.set_cinematic_mode(cinematic);
        spdlog::info("Cinematic mode: {}", cinematic ? "ON" : "OFF");

        if (!cinematic && world.is_global_render_mode())
        {
            world.set_global_render_mode(false);
            entities.set_global_render_mode(false);
            spdlog::info("Global render mode: OFF (cinematic disabled)");
        }
    }

    // Toggle tile debug overlay (F6)
    if (inp.is_key_pressed(sf::Keyboard::Key::F6))
    {
        auto config = world.render_config();
        config.show_walkability = !config.show_walkability;
        world.set_render_config(config);
        spdlog::info("Tile debug overlay: {}", config.show_walkability ? "ON" : "OFF");
    }

    // Toggle tile grid (F7)
    if (inp.is_key_pressed(sf::Keyboard::Key::F7))
    {
        auto config = world.render_config();
        config.show_grid = !config.show_grid;
        world.set_render_config(config);
        spdlog::info("Tile grid: {}", config.show_grid ? "ON" : "OFF");
    }

    // Cycle view mode for testing (F8), fog style (Shift+F8), targeting boundary (Ctrl+F8)
    if (inp.is_key_pressed(sf::Keyboard::Key::F8) && rend)
    {
        bool shift = inp.is_key_down(sf::Keyboard::Key::LShift) || inp.is_key_down(sf::Keyboard::Key::RShift);
        bool ctrl = inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl);

        if (ctrl)
        {
            // Ctrl+F8: toggle targeting boundary
            bool visible = !rend->is_targeting_boundary_visible();
            rend->set_targeting_boundary_visible(visible);
            spdlog::info("Targeting boundary: {}", visible ? "ON" : "OFF");
        }
        else if (shift)
        {
            // Shift+F8: cycle fog style
            auto current = rend->current_fog_style();
            fog_style next;
            const char* name;
            switch (current)
            {
            case fog_style::solid:
                next = fog_style::tile_fog;
                name = "tile_fog";
                break;
            case fog_style::tile_fog:
                next = fog_style::vignette;
                name = "vignette";
                break;
            case fog_style::vignette:
                next = fog_style::gradient;
                name = "gradient";
                break;
            default:
                next = fog_style::solid;
                name = "solid";
                break;
            }
            rend->set_fog_style(next);
            spdlog::info("Fog style: {}", name);
        }
        else
        {
            // F8: cycle view mode
            auto current = rend->current_view_mode();
            view_mode next;
            const char* name;
            switch (current)
            {
            case view_mode::special:
                next = view_mode::scaled;
                name = "scaled";
                break;
            case view_mode::scaled:
                next = view_mode::extended;
                name = "extended";
                break;
            default:
                next = view_mode::special;
                name = "special";
                break;
            }
            rend->set_view_mode(next);
            world.set_screen_size(rend->scene_width(), rend->scene_height());
            game_->send_view_range();
            spdlog::info("View mode: {} (fair zone: {}x{})", name, rend->scene_width(), rend->scene_height());
        }
    }

    // Cycle screen transition type (F9)
    if (inp.is_key_pressed(sf::Keyboard::Key::F9) && rend)
    {
        auto new_type = transition.next_type();
        transition.set_show_label(true);
        transition.start_reveal(rend->width(), rend->height());
        spdlog::info("Transition preview: {}", transition_type_name(new_type));
    }

    // Toggle zoom mode (Ctrl+Q)
    if (inp.is_key_pressed(sf::Keyboard::Key::Q) &&
        (inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl)))
    {
        world.set_zoom_mode_enabled(!world.is_zoom_mode_enabled());
        spdlog::info("Zoom mode: {}", world.is_zoom_mode_enabled() ? "ON" : "OFF");
    }

    // Toggle global render mode (Ctrl+G)
    if (inp.is_key_pressed(sf::Keyboard::Key::G) &&
        (inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl)))
    {
        if (world.is_cinematic_mode())
        {
            bool global = !world.is_global_render_mode();
            world.set_global_render_mode(global);
            entities.set_global_render_mode(global);
            spdlog::info("Global render mode: {}", global ? "ON" : "OFF");
        }
        else
        {
            spdlog::info("Global render mode requires cinematic mode (F5)");
        }
    }

    // Camera panning in cinematic mode
    if (world.is_cinematic_mode())
    {
        int32_t pan_amount = 32;
        if (inp.is_key_down(sf::Keyboard::Key::LShift) || inp.is_key_down(sf::Keyboard::Key::RShift))
        {
            pan_amount = 32 * 5;
        }

        if (inp.is_key_down(sf::Keyboard::Key::Left))
            world.move_camera(-pan_amount, 0);
        if (inp.is_key_down(sf::Keyboard::Key::Right))
            world.move_camera(pan_amount, 0);
        if (inp.is_key_down(sf::Keyboard::Key::Up))
            world.move_camera(0, -pan_amount);
        if (inp.is_key_down(sf::Keyboard::Key::Down))
            world.move_camera(0, pan_amount);
    }

    // Toggle run mode (Ctrl+R)
    if ((inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl)) &&
        inp.is_key_pressed(sf::Keyboard::Key::R))
    {
        run_mode_enabled_ = !run_mode_enabled_;
        spdlog::debug("Run mode: {}", run_mode_enabled_ ? "enabled" : "disabled");
    }

    // Toggle dialogs
    if (inp.is_key_pressed(sf::Keyboard::Key::C))
        ui.toggle_dialog(dialog_type::character_info);
    if (inp.is_key_pressed(sf::Keyboard::Key::I))
        ui.toggle_dialog(dialog_type::inventory);
    if (inp.is_key_pressed(sf::Keyboard::Key::E))
        ui.toggle_dialog(dialog_type::equipment);
    if (inp.is_key_pressed(sf::Keyboard::Key::K))
        ui.toggle_dialog(dialog_type::skills);
    if (inp.is_key_pressed(sf::Keyboard::Key::M))
        ui.toggle_dialog(dialog_type::spellbook);
    if (inp.is_key_pressed(sf::Keyboard::Key::P))
        ui.toggle_dialog(dialog_type::party);
    if (inp.is_key_pressed(sf::Keyboard::Key::G))
    {
        ui.toggle_dialog(dialog_type::guild);
        // Request fresh guild info when opening
        if (ui.is_dialog_open(dialog_type::guild) && game_->guild().in_guild())
            game_->ws_handler().request_guild_info();
    }

    // Fishing (F key)
    if (inp.is_key_pressed(sf::Keyboard::Key::F))
    {
        game_->ws_handler().request_fish_skill();
    }

    // Toggle force attack mode (Ctrl+A) - dash attacks without holding Ctrl
    if ((inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl)) &&
        inp.is_key_pressed(sf::Keyboard::Key::A))
    {
        force_attack_mode_ = !force_attack_mode_;
        spdlog::debug("Force attack mode: {}", force_attack_mode_ ? "ON" : "OFF");
    }

    // Toggle attack mode (Tab)
    if (inp.is_key_pressed(sf::Keyboard::Key::Tab))
    {
        game_->toggle_combat_mode();
        spdlog::debug("Combat mode toggled: {}", combat_mode_ ? "attack" : "peace");

        entity* player = game_->local_player();
        if (player && !player->transform().moving)
        {
            player->set_action_with_combat_mode(object_action::stop_peace, combat_mode_);
        }
    }

    // Toggle safe attack mode (Home)
    if (inp.is_key_pressed(sf::Keyboard::Key::Home))
    {
        safe_attack_mode_ = !safe_attack_mode_;
        spdlog::debug("Safe attack mode toggled: {}", safe_attack_mode_ ? "safe" : "PK");
    }

    // Help dialog (F1)
    if (inp.is_key_pressed(sf::Keyboard::Key::F1))
    {
        ui.toggle_dialog(dialog_type::help);
    }

    // Debug: Test event log colors with Ctrl+Number keys
    bool ctrl_held = inp.is_key_down(sf::Keyboard::Key::LControl) || inp.is_key_down(sf::Keyboard::Key::RControl);
    bool shift_held = inp.is_key_down(sf::Keyboard::Key::LShift) || inp.is_key_down(sf::Keyboard::Key::RShift);
    if (ctrl_held && !shift_held)
    {
        if (inp.is_key_pressed(sf::Keyboard::Key::Num1))
            status_log.add_event("White: Default message color", message_color::white);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num2))
            status_log.add_event("Green: You gained 150 experience!", message_color::green);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num3))
            status_log.add_event("Red: You took 47 damage!", message_color::red);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num4))
            status_log.add_event("Blue: Magic Missile hits for 32 damage", message_color::blue);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num5))
            status_log.add_event("Yellow: Warning - low health!", message_color::yellow);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num6))
            status_log.add_event("System: Server maintenance in 10 minutes", message_color::system);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num7))
            status_log.add_event("Rainbow cycling color effect!", message_color::rainbow);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num8))
            status_log.add_event("Special per-letter rainbow gradient!", message_color::special);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num9))
            status_log.add_event("Terror shaky bouncing text!", message_color::terror);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num0))
        {
            status_log.clear_events();
            spdlog::info("Status log events cleared");
        }
    }

    if (ctrl_held && shift_held)
    {
        if (inp.is_key_pressed(sf::Keyboard::Key::Num1))
            status_log.add_event("Pulsing alert - look at me!", message_color::pulsing);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num2))
            status_log.add_event("Wave ripple flowing text!", message_color::wave);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num3))
            status_log.add_event("Gl1tch c0rrupt3d d4ta str3am!", message_color::glitch);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num4))
            status_log.add_event("Typewriter: a message from the ancients...", message_color::typewriter);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num5))
            status_log.add_event("Dissolve burning away into nothing...", message_color::dissolve);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num6))
            status_log.add_event("Chromatic aberration interference!", message_color::chromatic);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num7))
            status_log.add_event("Glowing ethereal magic text!", message_color::glow);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num8))
            status_log.add_event("Heat distortion shimmer effect!", message_color::distortion);
        if (inp.is_key_pressed(sf::Keyboard::Key::Num9))
            status_log.add_event("Retro CRT scanline display!", message_color::scanlines);
        if (inp.is_key_pressed(sf::Keyboard::Key::F))
        {
            float wx, wy;
            if (auto* player = game_->local_player())
            {
                auto& t = player->transform();
                wx = static_cast<float>(t.x);
                wy = static_cast<float>(t.y);
            }
            else
            {
                wx = static_cast<float>(world.camera_x() + static_cast<int32_t>(rend->width()) / 2);
                wy = static_cast<float>(world.camera_y() + static_cast<int32_t>(rend->height()) / 2);
            }
            floating_text.add_damage(47, wx, wy - 20.0f);
            floating_text.add_heal(25, wx + 30.0f, wy - 10.0f);
            floating_text.add_critical(128, wx - 20.0f, wy - 30.0f);
            status_log.add_event("Floating text spawned!", message_color::yellow);
        }
    }
}

void input_handler::execute_dash_attack(entity* target, const input& /*inp*/)
{
    entity* player = game_->local_player();
    if (!player)
        return;

    auto& t = player->transform();
    auto& world = game_->game_world();
    auto& entities = game_->entities();

    // Get direct direction toward target (straight line, no obstacle avoidance)
    auto direct_dir = get_next_move_dir(t.tile_x, t.tile_y, target->transform().tile_x, target->transform().tile_y);
    if (!direct_dir)
        return;

    // Passability check for the intermediate tile (1 step toward target)
    auto is_passable = [&](int32_t x, int32_t y) -> bool
    {
        if (!world.current_map().is_walkable(x, y))
            return false;
        for (auto* e : entities.get_entities_on_tile(x, y))
        {
            if (e && e->is_alive() && e != player)
                return false;
        }
        return true;
    };

    // Try direct direction first, then CW and CCW rotations
    std::optional<direction> dash_dir;
    auto [ddx, ddy] = direction_offset(*direct_dir);
    if (is_passable(t.tile_x + ddx, t.tile_y + ddy))
    {
        dash_dir = *direct_dir;
    }
    else
    {
        // Try CW rotation
        direction cw = rotate_cw(*direct_dir);
        auto [cwx, cwy] = direction_offset(cw);
        if (is_passable(t.tile_x + cwx, t.tile_y + cwy))
        {
            dash_dir = cw;
        }
        else
        {
            // Try CCW rotation
            direction ccw = rotate_ccw(*direct_dir);
            auto [ccwx, ccwy] = direction_offset(ccw);
            if (is_passable(t.tile_x + ccwx, t.tile_y + ccwy))
            {
                dash_dir = ccw;
            }
        }
    }

    if (!dash_dir)
        return; // All 3 directions blocked

    auto [dx, dy] = direction_offset(*dash_dir);
    int32_t dash_x = t.tile_x + dx;
    int32_t dash_y = t.tile_y + dy;

    // Set up movement to intermediate tile
    t.move_start_x = t.tile_x;
    t.move_start_y = t.tile_y;
    t.tile_x = dash_x;
    t.tile_y = dash_y;
    t.facing = *dash_dir;
    t.moving = true;
    t.move_progress = 0.0f;

    if (player->has_movement())
    {
        player->movement().running = false;
        player->movement().target_x = -1;
        player->movement().target_y = -1;
    }

    // Dash uses the same attack stance as current combat mode (peace or combat).
    // set_action_with_combat_mode sets current_action_ for sprite rendering, then we override
    // the anim state to attack_move for the 13-frame timing (sped up attack + hold at end).
    player->set_action_with_combat_mode(object_action::attack_peace, combat_mode_);
    player->animation().set_state(entity_anim_state::attack_move);

    // Clear pathfinding destination
    move_dest_x_ = -1;
    move_dest_y_ = -1;

    // Send move request (to advance tile)
    uint8_t dir_protocol = static_cast<uint8_t>(direction_to_protocol(*dash_dir));
    json move_msg = make_player_move_request(t.move_start_x, t.move_start_y, dir_protocol, false, dash_x, dash_y);
    game_->ws_connection().send(move_msg);

    // Send attack request with dash attack type
    game_->ws_handler().request_attack(target->id(), static_cast<uint8_t>(attack_type::dash));

    spdlog::debug("Dash attack: moving ({},{}) -> ({},{}) and attacking entity {}",
                  t.move_start_x,
                  t.move_start_y,
                  dash_x,
                  dash_y,
                  target->id());
}

} // namespace hb
