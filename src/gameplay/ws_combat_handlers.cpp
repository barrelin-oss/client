#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "ui/dialogs/death_dialog.hpp"
#include "ui/dialogs/spellbook_dialog.hpp"
#include "audio/sound_types.hpp"
#include "core/direction_utils.hpp"
#include "graphics/color_utils.hpp"
#include "world/tile.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

void ws_message_handler::request_combat_mode_toggle()
{
    json msg = make_combat_mode_change_request();
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_attack(uint32_t target_id, uint8_t attack_type)
{
    auto* player = game_->local_player();
    if (!player)
        return;

    const auto& t = player->transform();

    // Determine target type: 2 = npc/monster, 1 = player
    uint8_t target_type = 1;
    entity* target = game_->entities().find(target_id);
    if (target && (target->has_npc() || target->has_monster()))
        target_type = 2;

    // Calculate direction toward target (don't rely on t.facing which may be stale)
    uint8_t dir = static_cast<uint8_t>(direction_to_protocol(t.facing));
    if (target)
    {
        auto calc_dir = calculate_direction(t.tile_x, t.tile_y, target->transform().tile_x, target->transform().tile_y);
        if (calc_dir)
            dir = static_cast<uint8_t>(direction_to_protocol(*calc_dir));
    }

    spdlog::info("Requesting attack on entity {} (type {} target_type {})", target_id, attack_type, target_type);
    json msg = make_player_attack_request(target_id, target_type, t.tile_x, t.tile_y, attack_type, dir);
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_magic(uint16_t spell_id, int32_t target_x, int32_t target_y, uint32_t target_id)
{
    auto* player = game_->local_player();
    if (!player)
        return;

    const auto& t = player->transform();

    // Determine target_type string based on target entity
    std::string target_type_str = "none";
    if (target_id != 0)
    {
        entity* target = game_->entities().find(target_id);
        if (target)
        {
            if (target->has_npc() || target->has_monster())
                target_type_str = "npc";
            else
                target_type_str = "player";
        }
    }
    else if (target_x >= 0 && target_y >= 0)
    {
        target_type_str = "ground";
    }

    uint8_t dir = static_cast<uint8_t>(direction_to_protocol(t.facing));

    spdlog::debug("Requesting magic: spell={} target=({},{}) entity={} type={}",
                  spell_id,
                  target_x,
                  target_y,
                  target_id,
                  target_type_str);
    json msg =
        make_player_magic_request(t.tile_x, t.tile_y, dir, spell_id, target_type_str, target_id, target_x, target_y);
    game_->ws_connection().send(msg);
}

void ws_message_handler::handle_player_magic_response(const json& message)
{
    auto data = player_magic_response_data::from_json(message);

    if (!data.success)
    {
        if (!data.error.empty())
        {
            game_->get_status_log().add_event(data.error, message_color::red);
            spdlog::debug("Magic failed: {}", data.error);
        }
        return;
    }

    auto& entities = game_->entities();
    entity* player = game_->local_player();
    if (!player)
        return;

    // Update local player MP (server-authoritative)
    player->stats().mp = data.caster_mp;

    // Look up spell definition
    const spell* sp = game_->magic().get_spell(data.spell_id);

    // Get target position for floating text
    float target_wx = static_cast<float>(player->transform().x);
    float target_wy = static_cast<float>(player->transform().y);

    if (data.target_id != 0)
    {
        entity* target = entities.find(data.target_id);
        if (target)
        {
            target_wx = static_cast<float>(target->transform().x);
            target_wy = static_cast<float>(target->transform().y);
        }
    }

    // Floating text for damage/heal
    if (data.damage > 0)
    {
        game_->floating_text().add_damage(data.damage, target_wx, target_wy);
    }
    if (data.heal > 0)
    {
        game_->floating_text().add_heal(data.heal, target_wx, target_wy);
    }

    // Status log message
    std::string spell_name = sp ? sp->name : ("Spell #" + std::to_string(data.spell_id));
    if (data.damage > 0)
    {
        game_->get_status_log().add_event(spell_name + " hits for " + std::to_string(data.damage) + " damage",
                                          message_color::blue);
    }
    else if (data.heal > 0)
    {
        game_->get_status_log().add_event(spell_name + " restores " + std::to_string(data.heal) + " HP",
                                          message_color::green);
    }
    else
    {
        game_->get_status_log().add_event(spell_name + " cast successfully", message_color::blue);
    }

    // Update target animation (HP is updated by entity_hp_update)
    if (data.target_id != 0)
    {
        entity* target = entities.find(data.target_id);
        if (target && target->is_alive() && data.damage > 0)
        {
            target->set_action(object_action::damage);
        }
    }

    spdlog::debug("Magic response: spell={} damage={} heal={} target={} mp={}",
                  data.spell_id,
                  data.damage,
                  data.heal,
                  data.target_id,
                  data.caster_mp);
}

void ws_message_handler::handle_spell_list_update(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    if (!d.contains("spells") || !d["spells"].is_array())
        return;

    auto& magic = game_->magic();

    for (const auto& sp : d["spells"])
    {
        auto spell_data = enter_game_spell::from_json(sp);
        magic.learn_spell(spell_data.spell_id);
        magic.set_spell_mastery(spell_data.spell_id, static_cast<uint8_t>(spell_data.level));
        magic.set_spell_total_casts(spell_data.spell_id, spell_data.total_casts);
    }

    // Refresh spellbook dialog if open
    if (auto* spell_dlg = dynamic_cast<spellbook_dialog*>(game_->ui().get_dialog(dialog_type::spellbook)))
    {
        spell_dlg->clear_spells();
        for (const auto* sp : magic.get_all_spells())
        {
            spell_dlg->add_spell(*sp);
        }
    }

    // Re-populate hotbar
    game_->auto_populate_spell_hotbar();

    spdlog::info("Spell list updated: {} spells received", d["spells"].size());
}

void ws_message_handler::handle_combat_attack_broadcast(const json& message)
{
    auto data = combat_attack_broadcast_data::from_json(message);

    auto& entities = game_->entities();

    // Set attacker animation and facing
    entity* attacker = entities.find(data.attacker_id);
    if (attacker)
    {
        // Use server-authoritative direction if provided, otherwise calculate from positions
        auto dir = direction_from_protocol(data.direction);
        if (!dir)
            dir = calculate_direction(data.attacker_x, data.attacker_y, data.target_x, data.target_y);
        if (dir)
            attacker->transform().facing = *dir;

        // Local player handles their own attack animation client-side (input_handler.cpp)
        if (attacker->id() != entities.local_player_id())
        {
            if (data.is_ranged())
                attacker->set_action(object_action::attack_combat_bow);
            else
                attacker->set_action_with_combat_mode(object_action::attack_peace,
                                                     attacker->has_combat() && attacker->combat().combat_stance);
        }

        attacker->set_target(data.target_id);
    }

    // Spawn arrow projectile for ranged attacks
    if (data.is_ranged() && !data.projectile_type.empty())
    {
        game_->effects().add_effect(
            effect_type_id::arrow, data.attacker_x, data.attacker_y, data.target_x, data.target_y);
    }

    // Apply damage to target
    entity* target = entities.find(data.target_id);
    if (target && data.hit && data.damage > 0)
    {
        // Damage interrupts everything — snap to destination if mid-movement, then flinch
        if (target->is_alive())
        {
            auto& tt = target->transform();
            if (tt.moving)
            {
                tt.move_start_x = tt.tile_x;
                tt.move_start_y = tt.tile_y;
                tt.x = tt.tile_x * tile_width + 16;
                tt.y = tt.tile_y * tile_height + 16;
                tt.move_progress = 0.0f;
                tt.moving = false;
            }
            target->set_action(object_action::damage);
        }

        const auto& t = target->transform();
        if (data.critical)
            game_->floating_text().add_critical(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
        else
            game_->floating_text().add_damage(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
    }
    else if (target && data.hit && data.damage == 0)
    {
        const auto& t = target->transform();
        game_->floating_text().add_damage(0, static_cast<float>(t.x), static_cast<float>(t.y));
    }
    else if (target && !data.hit)
    {
        // Show MISS floating text
        const auto& t = target->transform();
        game_->floating_text().add_text(
            "MISS", static_cast<float>(t.x), static_cast<float>(t.y), sf::Color(180, 180, 180));
    }

    // Store weapon impact sound on target for frame-triggered playback.
    // Legacy MapData.cpp plays C5/C6/C7 at frame 4 of DEF_OBJECTDAMAGE/DEF_OBJECTDYING
    // (the composite split point). process_damage_effects() plays it at the right frame.
    if (data.hit && data.damage > 0 && target)
    {
        weapon_type wt = weapon_type::none;
        if (data.attacker_id == entities.local_player_id())
        {
            if (const auto* weapon = game_->inventory().get_equipped_item(equip_pos::weapon))
                wt = weapon->weapon;
        }
        else if (data.is_ranged())
        {
            wt = weapon_type::bow;
        }
        target->animation().pending_impact_sound = get_weapon_impact_sound(wt);
    }

    spdlog::debug("Combat broadcast: {} -> {} ({} {} dmg={})",
                  data.attacker_id,
                  data.target_id,
                  data.attack_mode,
                  data.hit ? "HIT" : "MISS",
                  data.damage);
}

void ws_message_handler::handle_player_attack_response(const json& message)
{
    auto data = player_attack_response_data::from_json(message);

    if (!data.success)
    {
        if (!data.error.empty())
        {
            game_->get_status_log().add_event(data.error, message_color::red);
            spdlog::debug("Attack failed: {}", data.error);
        }

        // Reset local player to idle on attack rejection
        if (entity* player = game_->local_player())
        {
            player->set_action_with_combat_mode(object_action::stop_peace, game_->is_combat_mode());
        }
    }
}

void ws_message_handler::handle_npc_attack(const json& message)
{
    auto data = npc_attack_data::from_json(message);

    auto& entities = game_->entities();

    // Set NPC attacker animation and facing
    entity* attacker = entities.find(data.npc_id);
    if (attacker)
    {
        auto dir = calculate_direction(data.npc_x, data.npc_y, data.target_x, data.target_y);
        if (dir)
            attacker->transform().facing = *dir;

        if (data.is_ranged)
            attacker->set_action(object_action::attack_combat_bow);
        else
            attacker->set_action(object_action::attack_peace);

        attacker->set_target(data.target_id);
    }

    // Spawn arrow projectile for ranged NPC attacks
    if (data.is_ranged && !data.projectile_type.empty())
    {
        game_->effects().add_effect(effect_type_id::arrow, data.npc_x, data.npc_y, data.target_x, data.target_y);
    }

    // Apply damage to target
    entity* target = entities.find(data.target_id);
    if (target && data.hit() && data.damage > 0)
    {
        // Damage interrupts everything — snap to destination if mid-movement, then flinch
        if (target->is_alive())
        {
            auto& tt = target->transform();
            if (tt.moving)
            {
                tt.move_start_x = tt.tile_x;
                tt.move_start_y = tt.tile_y;
                tt.x = tt.tile_x * tile_width + 16;
                tt.y = tt.tile_y * tile_height + 16;
                tt.move_progress = 0.0f;
                tt.moving = false;
            }
            target->set_action(object_action::damage);
        }

        const auto& t = target->transform();
        if (data.critical)
            game_->floating_text().add_critical(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
        else
            game_->floating_text().add_damage(data.damage, static_cast<float>(t.x), static_cast<float>(t.y));
    }
    else if (target && !data.hit())
    {
        // Show MISS floating text
        const auto& t = target->transform();
        game_->floating_text().add_text(
            "MISS", static_cast<float>(t.x), static_cast<float>(t.y), sf::Color(180, 180, 180));
    }

    spdlog::debug("NPC attack: {} -> {} ({} {} dmg={})",
                  data.npc_id,
                  data.target_id,
                  data.is_ranged ? "ranged" : "melee",
                  data.hit() ? "HIT" : "MISS",
                  data.damage);
}

void ws_message_handler::handle_entity_death(const json& message)
{
    auto data = entity_death_data::from_json(message);

    auto& entities = game_->entities();
    entity* victim = entities.find(data.victim_id);
    if (!victim)
        return;

    // NPCs/monsters can only die once; players can "die" multiple times (pretend corpse)
    bool is_npc = (victim->type() == entity_type::npc || victim->type() == entity_type::monster);
    if (is_npc && !victim->is_alive())
        return;

    if (victim->has_stats())
        victim->stats().hp = 0;

    // Snap to destination if mid-movement
    auto& t = victim->transform();
    if (t.moving)
    {
        t.move_start_x = t.tile_x;
        t.move_start_y = t.tile_y;
        t.x = t.tile_x * tile_width + 16;
        t.y = t.tile_y * tile_height + 16;
        t.move_progress = 0.0f;
        t.moving = false;
    }

    // Store weapon impact sound for the killing blow (played at frame 4 of dying).
    // Legacy MapData.cpp plays C5/C6/C7 at frame 4 of DEF_OBJECTDYING.
    {
        weapon_type wt = weapon_type::none;
        if (data.killer_id == entities.local_player_id())
        {
            if (const auto* weapon = game_->inventory().get_equipped_item(equip_pos::weapon))
                wt = weapon->weapon;
        }
        victim->animation().pending_impact_sound = get_weapon_impact_sound(wt);
    }

    // Transition to dead immediately, then play the dying animation.
    // The dying animation renders idle sprite for frames 0-3, then the actual
    // death animation for frames 4+, matching legacy DrawObject_OnDying behavior.
    game_->entities().transition_to_dead(data.victim_id);
    victim->set_action(object_action::dying);

    if (is_npc)
    {
        std::string name = victim->has_name() ? victim->name().name : "unknown";
        uint16_t npc_type = 0;
        std::string category;
        if (victim->has_npc())
        {
            npc_type = victim->npc().npc_type;
        }
        if (victim->has_monster())
        {
            npc_type = victim->monster().monster_type;
            category = victim->monster().category;
        }
        const auto& t = victim->transform();
        spdlog::debug("NPC death: id={} name='{}' type={} category='{}' "
                      "tile=({},{}) world=({},{}) killer={} facing={} alive_was=true",
                      data.victim_id,
                      name,
                      npc_type,
                      category,
                      t.tile_x,
                      t.tile_y,
                      t.x,
                      t.y,
                      data.killer_id,
                      static_cast<int>(t.facing));
    }
    else
    {
        spdlog::debug("Entity death: victim={} killer={} at ({},{})", data.victim_id, data.killer_id, data.x, data.y);
    }
}

void ws_message_handler::handle_combat_effect(const json& message)
{
    auto data = combat_effect_data::from_json(message);

    auto& entities = game_->entities();

    // Skip effects where we are the source - we already handle our own
    // attacks/spells via player_attack_response / player_magic_response
    if (data.source_id != 0 && data.source_id == entities.local_player_id())
    {
        spdlog::debug("Skipping combat_effect for local player source");
        return;
    }

    // Determine world position for floating text
    float wx = static_cast<float>(data.target_x * 32 + 16);
    float wy = static_cast<float>(data.target_y * 32 + 16);

    // Try to get more accurate position from the target entity
    if (entity* target = entities.find(data.target_id))
    {
        wx = static_cast<float>(target->transform().x);
        wy = static_cast<float>(target->transform().y);
    }

    // Spell visual effects — only for actual spells (spell_id > 0)
    if (data.spell_id > 0)
    {
        const spell* sp = game_->magic().get_spell(data.spell_id);

        // Set caster animation and spell name
        entity* caster = entities.find(data.source_id);
        if (caster)
        {
            caster->set_action(object_action::magic);

            if (sp && caster->has_name())
            {
                auto& name = caster->name();
                name.chat_message = sp->name + "!";
                name.chat_timer = 3.0f;
                name.chat_elapsed = 0.0f;
                name.chat_style = {sf::Color::Red, sf::Color::Black, 1.0f, 14};
            }
        }

        // Spawn projectile and/or impact effects using world pixel coords
        if (sp)
        {
            // Caster world position
            float src_wx = static_cast<float>(data.target_x * 32 + 16);
            float src_wy = static_cast<float>(data.target_y * 32 + 16);
            if (caster)
            {
                src_wx = static_cast<float>(caster->transform().x);
                src_wy = static_cast<float>(caster->transform().y);
            }

            // Target world position
            float dest_wx = static_cast<float>(data.target_x * 32 + 16);
            float dest_wy = static_cast<float>(data.target_y * 32 + 16);
            if (data.target_id != 0)
            {
                entity* target = entities.find(data.target_id);
                if (target)
                {
                    dest_wx = static_cast<float>(target->transform().x);
                    dest_wy = static_cast<float>(target->transform().y);
                }
            }

            if (sp->projectile_effect != 0)
            {
                game_->effects().add_effect_world(
                    static_cast<effect_type_id>(sp->projectile_effect), src_wx, src_wy, dest_wx, dest_wy);
            }
            else if (sp->effect_sprite != 0)
            {
                // Area/ground spells always render at the target tile position
                // Single-target spells with no target entity render on the caster
                float fx = dest_wx;
                float fy = dest_wy;
                if (data.target_id == 0 && sp->target_type == spell_target::single)
                {
                    fx = src_wx;
                    fy = src_wy;
                }
                game_->effects().add_effect_at_pixel(static_cast<effect_type_id>(sp->effect_sprite), fx, fy);
            }
        }
    }

    // Floating text
    auto& ft = game_->floating_text();

    if (data.effect_type == "damage")
    {
        if (data.is_critical)
            ft.add_critical(data.value, wx, wy);
        else
            ft.add_damage(data.value, wx, wy);
    }
    else if (data.effect_type == "heal")
    {
        ft.add_heal(data.value, wx, wy);
    }
    else if (data.effect_type == "miss")
    {
        ft.add_text("MISS", wx, wy, sf::Color(180, 180, 180));
    }
    else if (data.effect_type == "dodge")
    {
        ft.add_text("DODGE", wx, wy, sf::Color(120, 180, 255));
    }
    else if (data.effect_type == "block")
    {
        ft.add_text("BLOCK", wx, wy, sf::Color(180, 180, 180));
    }
    else if (data.effect_type == "resist")
    {
        ft.add_text("RESIST", wx, wy, sf::Color(180, 100, 255));
    }
    else
    {
        spdlog::debug("Unhandled combat effect type: {}", data.effect_type);
    }

    // Update target animation (HP is updated by entity_hp_update)
    if (data.target_id != 0)
    {
        entity* target = entities.find(data.target_id);
        if (target && target->is_alive() && data.effect_type == "damage" && data.value > 0)
        {
            target->set_action(object_action::damage);
        }
    }

    spdlog::debug("Combat effect: {} -> {} type={} value={} spell={} critical={}",
                  data.source_id,
                  data.target_id,
                  data.effect_type,
                  data.value,
                  data.spell_id,
                  data.is_critical);
}

void ws_message_handler::handle_player_death_info(const json& message)
{
    auto data = player_death_info_data::from_json(message);

    // Create or reuse death dialog
    auto& ui = game_->ui();
    auto* dlg = dynamic_cast<death_dialog*>(ui.get_dialog(dialog_type::death));
    if (!dlg)
    {
        auto new_dlg = std::make_unique<death_dialog>();
        dlg = new_dlg.get();
        ui.add_dialog(dialog_type::death, std::move(new_dlg));
    }

    dlg->set_death_info(data.killer_name, data.is_pvp, data.xp_lost);
    dlg->set_on_restart(
        [this]()
        {
            game_->ui().close_dialog(dialog_type::death);
            json msg = make_player_respawn_request();
            game_->ws_connection().send(msg);
            spdlog::info("Sent player respawn request");
        });
    dlg->set_on_resurrect(
        [this]()
        {
            // TODO: send accept resurrection request
            spdlog::info("Accept resurrection requested (not yet implemented)");
        });
    dlg->open();

    // Mark local player as dead and play dying animation
    if (entity* player = game_->local_player())
    {
        player->set_alive(false);
        if (player->has_stats())
            player->stats().hp = 0;
        player->set_action(object_action::dying);
    }

    spdlog::info("Player died! Killer: {} (pvp={}) XP lost: {} Respawn: {} ({},{})",
                 data.killer_name,
                 data.is_pvp,
                 data.xp_lost,
                 data.respawn_map,
                 data.respawn_x,
                 data.respawn_y);
}

void ws_message_handler::handle_respawn_response(const json& message)
{
    if (!message.contains("data"))
        return;
    const auto& d = message["data"];

    bool success = d.value("success", false);
    if (!success)
    {
        std::string error = d.value("error", "unknown");
        spdlog::warn("Respawn failed: {}", error);
        return;
    }

    // Success — the server will follow up with a player_teleport message
    // which handles map change, position reset, and reviving the player.
    spdlog::info("Respawn accepted, awaiting teleport");
}

void ws_message_handler::handle_combat_mode_change_broadcast(const json& message)
{
    auto data = combat_mode_change_broadcast_data::from_json(message);

    auto& entities = game_->entities();
    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
        return;

    if (!ent->has_combat())
        return;

    ent->combat().combat_stance = data.combat_mode;

    // Update current animation to reflect the stance change
    auto& anim = ent->animation();
    if (anim.state == entity_anim_state::stop)
    {
        ent->set_action(data.combat_mode ? object_action::stop_combat : object_action::stop_peace);
    }
    else if (anim.state == entity_anim_state::move)
    {
        ent->set_action(data.combat_mode ? object_action::move_combat : object_action::move_peace);
    }

    spdlog::debug("Entity {} combat stance: {}", data.entity_id, data.combat_mode);
}

void ws_message_handler::handle_combat_mode_change_response(const json& message)
{
    auto data = combat_mode_change_response_data::from_json(message);

    // Apply the server-confirmed combat mode to the local player
    auto& input = game_->input_handler();
    input.set_combat_mode(data.combat_mode);

    entity* player = game_->local_player();
    if (player)
    {
        if (player->has_combat())
            player->combat().combat_stance = data.combat_mode;

        auto& anim = player->animation();
        if (anim.state == entity_anim_state::stop)
        {
            player->set_action(data.combat_mode ? object_action::stop_combat : object_action::stop_peace);
        }
        else if (anim.state == entity_anim_state::move)
        {
            player->set_action(data.combat_mode ? object_action::move_combat : object_action::move_peace);
        }
    }

    spdlog::debug("Combat mode confirmed: {}", data.combat_mode);
}

void ws_message_handler::handle_super_attack_update(const json& message)
{
    if (!message.contains("data"))
        return;
    int32_t charges = message["data"].value("charges", 0);
    if (entity* player = game_->local_player(); player && player->has_stats())
        player->stats().super_attack_charges = charges;
    if (auto* panel = game_->ui().get_icon_panel())
        panel->set_super_attack_count(charges);
    spdlog::info("Super attack charges: {}", charges);
}

void ws_message_handler::handle_player_action_broadcast(const json& message)
{
    auto data = player_action_broadcast_data::from_json(message);
    auto& entities = game_->entities();

    // Don't apply to local player — we already play our own animations
    if (data.entity_id == entities.local_player_id())
        return;

    entity* ent = entities.get_entity(data.entity_id);
    if (!ent)
        return;

    auto& t = ent->transform();
    t.facing = direction_from_protocol(data.direction).value_or(t.facing);

    if (data.action == "attack")
    {
        bool combat = ent->has_combat() && ent->combat().combat_stance;
        ent->set_action_with_combat_mode(object_action::attack_peace, combat);
    }
    else if (data.action == "dash_attack")
    {
        bool combat = ent->has_combat() && ent->combat().combat_stance;
        ent->set_action_with_combat_mode(object_action::attack_peace, combat);
        ent->animation().set_state(entity_anim_state::attack_move);
    }
    else if (data.action == "super_attack")
    {
        bool combat = ent->has_combat() && ent->combat().combat_stance;
        ent->set_action_with_combat_mode(object_action::attack_peace, combat);
        game_->combat().play_super_attack_shout(ent->id());
    }
    else if (data.action == "magic")
    {
        ent->set_action(object_action::magic);
    }
    else if (data.action == "pickup")
    {
        ent->set_action(object_action::get_item);
    }

    spdlog::debug("Entity {} action broadcast: {} dir={}", data.entity_id, data.action, data.direction);
}

} // namespace hb
