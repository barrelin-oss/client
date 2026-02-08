#include "network/handlers/notify_handlers.hpp"
#include "gameplay/game_state.hpp"
#include "ui/dialogs/dialogs.hpp"
#include <spdlog/spdlog.h>

namespace hb {

void notify_handler::initialize(game_state_manager& game) {
    game_ = &game;
    register_handlers();
    spdlog::info("Notify handler initialized with {} handlers", handlers_.size());
}

void notify_handler::register_handlers() {
    // Stats
    handlers_[notify_type::hp] = [this](game_state_manager&, packet_reader& r) { handle_hp(r); };
    handlers_[notify_type::mp] = [this](game_state_manager&, packet_reader& r) { handle_mp(r); };
    handlers_[notify_type::sp] = [this](game_state_manager&, packet_reader& r) { handle_sp(r); };
    handlers_[notify_type::exp] = [this](game_state_manager&, packet_reader& r) { handle_exp(r); };
    handlers_[notify_type::level_up] = [this](game_state_manager&, packet_reader& r) { handle_level_up(r); };
    handlers_[notify_type::charisma] = [this](game_state_manager&, packet_reader& r) { handle_charisma(r); };
    handlers_[notify_type::hunger] = [this](game_state_manager&, packet_reader& r) { handle_hunger(r); };

    // Combat
    handlers_[notify_type::killed] = [this](game_state_manager&, packet_reader& r) { handle_killed(r); };
    handlers_[notify_type::damage_move] = [this](game_state_manager&, packet_reader& r) { handle_damage_move(r); };
    handlers_[notify_type::safe_attack_mode] = [this](game_state_manager&, packet_reader& r) { handle_safe_attack_mode(r); };
    handlers_[notify_type::super_attack_left] = [this](game_state_manager&, packet_reader& r) { handle_super_attack_left(r); };
    handlers_[notify_type::global_attack_mode] = [this](game_state_manager&, packet_reader& r) { handle_global_attack_mode(r); };

    // Magic/Skills
    handlers_[notify_type::skill] = [this](game_state_manager&, packet_reader& r) { handle_skill(r); };
    handlers_[notify_type::magic_effect_on] = [this](game_state_manager&, packet_reader& r) { handle_magic_effect_on(r); };
    handlers_[notify_type::magic_effect_off] = [this](game_state_manager&, packet_reader& r) { handle_magic_effect_off(r); };
    handlers_[notify_type::magic_study_success] = [this](game_state_manager&, packet_reader& r) { handle_magic_study_success(r); };
    handlers_[notify_type::magic_study_fail] = [this](game_state_manager&, packet_reader& r) { handle_magic_study_fail(r); };
    handlers_[notify_type::skill_train_success] = [this](game_state_manager&, packet_reader& r) { handle_skill_train_success(r); };
    handlers_[notify_type::skill_train_fail] = [this](game_state_manager&, packet_reader& r) { handle_skill_train_fail(r); };
    handlers_[notify_type::skill_using_end] = [this](game_state_manager&, packet_reader& r) { handle_skill(r); };

    // Items
    handlers_[notify_type::item_obtained] = [this](game_state_manager&, packet_reader& r) { handle_item_obtained(r); };
    handlers_[notify_type::drop_item_fin_erase_item] = [this](game_state_manager&, packet_reader& r) { handle_item_drop(r); };
    handlers_[notify_type::give_item_fin_erase_item] = [this](game_state_manager&, packet_reader& r) { handle_item_drop(r); };
    handlers_[notify_type::item_depleted_erase_item] = [this](game_state_manager&, packet_reader& r) { handle_item_drop(r); };
    handlers_[notify_type::item_to_bank] = [this](game_state_manager&, packet_reader& r) { handle_item_to_bank(r); };
    handlers_[notify_type::cannot_item_to_bank] = [this](game_state_manager&, packet_reader& r) { handle_cannot_item_to_bank(r); };
    handlers_[notify_type::set_item_count] = [this](game_state_manager&, packet_reader& r) { handle_set_item_count(r); };
    handlers_[notify_type::item_lifespan_end] = [this](game_state_manager&, packet_reader& r) { handle_item_lifespan_end(r); };
    handlers_[notify_type::item_released] = [this](game_state_manager&, packet_reader& r) { handle_item_released(r); };
    handlers_[notify_type::item_color_change] = [this](game_state_manager&, packet_reader& r) { handle_item_color_change(r); };
    handlers_[notify_type::item_attribute_change] = [this](game_state_manager&, packet_reader& r) { handle_item_attribute_change(r); };
    handlers_[notify_type::cannot_carry_more_item] = [this](game_state_manager&, packet_reader& r) { handle_cannot_carry_more_item(r); };

    // Shop/Trade
    handlers_[notify_type::item_purchased] = [this](game_state_manager&, packet_reader& r) { handle_item_purchased(r); };
    handlers_[notify_type::not_enough_gold] = [this](game_state_manager&, packet_reader& r) { handle_not_enough_gold(r); };
    handlers_[notify_type::sell_item_price] = [this](game_state_manager&, packet_reader& r) { handle_sell_item_price(r); };
    handlers_[notify_type::cannot_sell_item] = [this](game_state_manager&, packet_reader& r) { handle_cannot_sell_item(r); };
    handlers_[notify_type::item_sold] = [this](game_state_manager&, packet_reader& r) { handle_item_sold(r); };
    handlers_[notify_type::repair_item_price] = [this](game_state_manager&, packet_reader& r) { handle_repair_item_price(r); };
    handlers_[notify_type::cannot_repair_item] = [this](game_state_manager&, packet_reader& r) { handle_cannot_repair_item(r); };
    handlers_[notify_type::item_repaired] = [this](game_state_manager&, packet_reader& r) { handle_item_repaired(r); };
    handlers_[notify_type::reward_gold] = [this](game_state_manager&, packet_reader& r) { handle_reward_gold(r); };

    // Exchange
    handlers_[notify_type::open_exchange_window] = [this](game_state_manager&, packet_reader& r) { handle_open_exchange_window(r); };
    handlers_[notify_type::set_exchange_item] = [this](game_state_manager&, packet_reader& r) { handle_set_exchange_item(r); };
    handlers_[notify_type::cancel_exchange_item] = [this](game_state_manager&, packet_reader& r) { handle_cancel_exchange_item(r); };
    handlers_[notify_type::exchange_item_complete] = [this](game_state_manager&, packet_reader& r) { handle_exchange_item_complete(r); };

    // Guild
    handlers_[notify_type::guild_disbanded] = [this](game_state_manager&, packet_reader& r) { handle_guild_disbanded(r); };
    handlers_[notify_type::new_guildsman] = [this](game_state_manager&, packet_reader& r) { handle_new_guildsman(r); };
    handlers_[notify_type::dismiss_guildsman] = [this](game_state_manager&, packet_reader& r) { handle_dismiss_guildsman(r); };
    handlers_[notify_type::query_join_guild_permission] = [this](game_state_manager&, packet_reader& r) { handle_query_join_guild_permission(r); };
    handlers_[notify_type::query_dismiss_guild_permission] = [this](game_state_manager&, packet_reader& r) { handle_query_dismiss_guild_permission(r); };
    handlers_[notify_type::wait_for_guild_operation] = [this](game_state_manager&, packet_reader& r) { handle_wait_for_guild_operation(r); };
    handlers_[notify_type::cannot_join_more_guildsman] = [this](game_state_manager&, packet_reader& r) { handle_cannot_join_more_guildsman(r); };

    // Party
    handlers_[notify_type::party] = [this](game_state_manager&, packet_reader& r) { handle_party(r); };
    handlers_[notify_type::response_create_new_party] = [this](game_state_manager&, packet_reader& r) { handle_response_create_new_party(r); };
    handlers_[notify_type::query_join_party] = [this](game_state_manager&, packet_reader& r) { handle_query_join_party(r); };

    // Quest
    handlers_[notify_type::quest_contents] = [this](game_state_manager&, packet_reader& r) { handle_quest_contents(r); };
    handlers_[notify_type::quest_completed] = [this](game_state_manager&, packet_reader& r) { handle_quest_completed(r); };
    handlers_[notify_type::quest_reward] = [this](game_state_manager&, packet_reader& r) { handle_quest_reward(r); };

    // Crafting
    handlers_[notify_type::build_item_success] = [this](game_state_manager&, packet_reader& r) { handle_build_item_success(r); };
    handlers_[notify_type::build_item_fail] = [this](game_state_manager&, packet_reader& r) { handle_build_item_fail(r); };
    handlers_[notify_type::portion_success] = [this](game_state_manager&, packet_reader& r) { handle_portion_success(r); };
    handlers_[notify_type::item_upgrade_fail] = [this](game_state_manager&, packet_reader& r) { handle_item_upgrade_fail(r); };

    // Fishing
    handlers_[notify_type::fish_chance] = [this](game_state_manager&, packet_reader& r) { handle_fish_chance(r); };
    handlers_[notify_type::fish_success] = [this](game_state_manager&, packet_reader& r) { handle_fish_success(r); };
    handlers_[notify_type::fish_fail] = [this](game_state_manager&, packet_reader& r) { handle_fish_fail(r); };

    // PK/Combat
    handlers_[notify_type::pk_penalty] = [this](game_state_manager&, packet_reader& r) { handle_pk_penalty(r); };
    handlers_[notify_type::pk_captured] = [this](game_state_manager&, packet_reader& r) { handle_pk_captured(r); };
    handlers_[notify_type::enemy_kill_reward] = [this](game_state_manager&, packet_reader& r) { handle_enemy_kill_reward(r); };
    handlers_[notify_type::enemy_kills] = [this](game_state_manager&, packet_reader& r) { handle_enemy_kills(r); };

    // Crusade/War
    handlers_[notify_type::crusade] = [this](game_state_manager&, packet_reader& r) { handle_crusade(r); };
    handlers_[notify_type::locked_map] = [this](game_state_manager&, packet_reader& r) { handle_locked_map(r); };
    handlers_[notify_type::duty_selected] = [this](game_state_manager&, packet_reader& r) { handle_duty_selected(r); };
    handlers_[notify_type::map_status_next] = [this](game_state_manager&, packet_reader& r) { handle_map_status_next(r); };
    handlers_[notify_type::map_status_last] = [this](game_state_manager&, packet_reader& r) { handle_map_status_last(r); };
    handlers_[notify_type::meteor_strike_coming] = [this](game_state_manager&, packet_reader& r) { handle_meteor_strike_coming(r); };
    handlers_[notify_type::meteor_strike_hit] = [this](game_state_manager&, packet_reader& r) { handle_meteor_strike_hit(r); };
    handlers_[notify_type::grand_magic_result] = [this](game_state_manager&, packet_reader& r) { handle_grand_magic_result(r); };

    // Special abilities
    handlers_[notify_type::energy_sphere_created] = [this](game_state_manager&, packet_reader& r) { handle_energy_sphere_created(r); };
    handlers_[notify_type::energy_sphere_goal_in] = [this](game_state_manager&, packet_reader& r) { handle_energy_sphere_goal_in(r); };
    handlers_[notify_type::special_ability_enabled] = [this](game_state_manager&, packet_reader& r) { handle_special_ability_enabled(r); };
    handlers_[notify_type::special_ability_status] = [this](game_state_manager&, packet_reader& r) { handle_special_ability_status(r); };

    // World events
    handlers_[notify_type::new_dynamic_object] = [this](game_state_manager&, packet_reader& r) { handle_new_dynamic_object(r); };
    handlers_[notify_type::del_dynamic_object] = [this](game_state_manager&, packet_reader& r) { handle_del_dynamic_object(r); };
    handlers_[notify_type::weather_change] = [this](game_state_manager&, packet_reader& r) { handle_weather_change(r); };
    handlers_[notify_type::time_change] = [this](game_state_manager&, packet_reader& r) { handle_time_change(r); };

    // System
    handlers_[notify_type::event_msg_string] = [this](game_state_manager&, packet_reader& r) { handle_event_msg_string(r); };
    handlers_[notify_type::server_shutdown] = [this](game_state_manager&, packet_reader& r) { handle_server_shutdown(r); };
    handlers_[notify_type::total_users] = [this](game_state_manager&, packet_reader& r) { handle_total_users(r); };
    handlers_[notify_type::force_disconn] = [this](game_state_manager&, packet_reader& r) { handle_force_disconn(r); };

    // Misc
    handlers_[notify_type::npc_talk] = [this](game_state_manager&, packet_reader& r) { handle_npc_talk(r); };
    handlers_[notify_type::player_profile] = [this](game_state_manager&, packet_reader& r) { handle_player_profile(r); };
    handlers_[notify_type::player_on_game] = [this](game_state_manager&, packet_reader& r) { handle_player_on_game(r); };
    handlers_[notify_type::player_not_on_game] = [this](game_state_manager&, packet_reader& r) { handle_player_not_on_game(r); };
    handlers_[notify_type::whisper_mode_on] = [this](game_state_manager&, packet_reader& r) { handle_whisper_mode_on(r); };
    handlers_[notify_type::whisper_mode_off] = [this](game_state_manager&, packet_reader& r) { handle_whisper_mode_off(r); };
    handlers_[notify_type::player_shutup] = [this](game_state_manager&, packet_reader& r) { handle_player_shutup(r); };
    handlers_[notify_type::to_be_recalled] = [this](game_state_manager&, packet_reader& r) { handle_to_be_recalled(r); };
    handlers_[notify_type::show_map] = [this](game_state_manager&, packet_reader& r) { handle_show_map(r); };
    handlers_[notify_type::server_change] = [this](game_state_manager&, packet_reader& r) { handle_server_change(r); };
    handlers_[notify_type::observer_mode] = [this](game_state_manager&, packet_reader& r) { handle_observer_mode(r); };
    handlers_[notify_type::limited_level] = [this](game_state_manager&, packet_reader& r) { handle_limited_level(r); };
}

void notify_handler::dispatch(uint16_t subtype, packet_reader& reader) {
    auto it = handlers_.find(subtype);
    if (it != handlers_.end() && game_) {
        it->second(*game_, reader);
    } else {
        spdlog::debug("Unhandled notification subtype: 0x{:04X}", subtype);
    }
}

// ============================================================================
// STAT HANDLERS
// ============================================================================

void notify_handler::handle_hp(packet_reader& reader) {
    auto hp = reader.read_i32();
    if (!hp || !game_) return;

    if (auto* player = game_->local_player()) {
        int32_t old_hp = player->stats().hp;
        player->stats().hp = *hp;

        // Cancel screen transition if player took damage
        if (*hp < old_hp && game_->is_transitioning())
        {
            game_->cancel_transition();
        }

        spdlog::debug("HP updated: {}", *hp);
    }
}

void notify_handler::handle_mp(packet_reader& reader) {
    auto mp = reader.read_i32();
    if (!mp || !game_) return;

    if (auto* player = game_->local_player()) {
        player->stats().mp = *mp;
        spdlog::debug("MP updated: {}", *mp);
    }
}

void notify_handler::handle_sp(packet_reader& reader) {
    auto sp = reader.read_i32();
    if (!sp || !game_) return;

    if (auto* player = game_->local_player()) {
        player->stats().sp = *sp;
        spdlog::debug("SP updated: {}", *sp);
    }
}

void notify_handler::handle_exp(packet_reader& reader) {
    auto exp = reader.read_u32();
    if (!exp || !game_) return;

    if (auto* player = game_->local_player()) {
        player->stats().exp = *exp;
        spdlog::debug("EXP updated: {}", *exp);
    }
}

void notify_handler::handle_level_up(packet_reader& reader) {
    auto level = reader.read_u16();
    if (!level || !game_) return;

    if (auto* player = game_->local_player()) {
        player->stats().level = *level;
        spdlog::info("Level up! Now level {}", *level);
        game_->show_message("Level Up!");
    }
}

void notify_handler::handle_charisma(packet_reader& reader) {
    auto chr = reader.read_u16();
    if (!chr || !game_) return;

    if (auto* player = game_->local_player()) {
        player->stats().charisma = *chr;
    }
}

void notify_handler::handle_hunger(packet_reader& reader) {
    auto hunger = reader.read_u8();
    if (!hunger || !game_) return;

    if (auto* player = game_->local_player()) {
        player->stats().hunger = *hunger;
    }
}

// ============================================================================
// COMBAT HANDLERS
// ============================================================================

void notify_handler::handle_killed(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Player was killed");
    game_->show_message("You have died!");
}

void notify_handler::handle_damage_move(packet_reader& reader) {
    auto entity_id = reader.read_u32();
    auto damage = reader.read_i32();
    auto dir = reader.read_u8();

    if (!entity_id || !damage || !dir || !game_) return;

    // Apply knockback effect to entity
    if (auto* ent = game_->entities().find(*entity_id)) {
        ent->set_action(object_action::damage);
        spdlog::debug("Entity {} took {} damage, knocked back", *entity_id, *damage);
    }
}

void notify_handler::handle_safe_attack_mode(packet_reader& reader) {
    auto mode = reader.read_u8();
    if (!mode || !game_) return;

    spdlog::info("Safe attack mode: {}", *mode ? "ON" : "OFF");
}

void notify_handler::handle_super_attack_left(packet_reader& reader) {
    auto count = reader.read_u8();
    if (!count || !game_) return;

    spdlog::debug("Super attacks left: {}", *count);
}

void notify_handler::handle_global_attack_mode(packet_reader& reader) {
    auto mode = reader.read_u8();
    if (!mode || !game_) return;

    spdlog::info("Global attack mode: {}", *mode);
}

// ============================================================================
// MAGIC/SKILL HANDLERS
// ============================================================================

void notify_handler::handle_skill(packet_reader& reader) {
    auto skill_id = reader.read_u8();
    auto mastery = reader.read_u8();

    if (!skill_id || !mastery || !game_) return;

    game_->skills().set_mastery(*skill_id, *mastery);
    spdlog::debug("Skill {} mastery: {}%", *skill_id, *mastery);
}

void notify_handler::handle_magic_effect_on(packet_reader& reader) {
    auto effect_id = reader.read_u16();
    auto duration = reader.read_u32();

    if (!effect_id || !game_) return;

    game_->magic().add_active_effect(*effect_id, duration.value_or(0));
    spdlog::debug("Magic effect {} activated", *effect_id);
}

void notify_handler::handle_magic_effect_off(packet_reader& reader) {
    auto effect_id = reader.read_u16();
    if (!effect_id || !game_) return;

    game_->magic().remove_active_effect(*effect_id);
    spdlog::debug("Magic effect {} deactivated", *effect_id);
}

void notify_handler::handle_magic_study_success(packet_reader& reader) {
    auto spell_id = reader.read_u16();
    if (!spell_id || !game_) return;

    game_->magic().learn_spell(*spell_id);
    spdlog::info("Learned spell {}", *spell_id);
    game_->show_message("Spell learned!");
}

void notify_handler::handle_magic_study_fail(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Failed to learn spell");
}

void notify_handler::handle_skill_train_success(packet_reader& reader) {
    auto skill_id = reader.read_u8();
    if (!skill_id || !game_) return;

    spdlog::info("Skill {} training successful", *skill_id);
    game_->show_message("Skill trained!");
}

void notify_handler::handle_skill_train_fail(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Failed to train skill");
}

// ============================================================================
// ITEM HANDLERS
// ============================================================================

void notify_handler::handle_item_obtained(packet_reader& reader) {
    auto slot = reader.read_u8();
    auto item_name = reader.read_string(32);

    if (!slot || !game_) return;

    spdlog::info("Obtained item in slot {}: {}", *slot, item_name.value_or("Unknown"));
}

void notify_handler::handle_item_drop(packet_reader& reader) {
    auto slot = reader.read_u8();
    if (!slot || !game_) return;

    game_->inventory().clear_slot(*slot);
    spdlog::debug("Item dropped from slot {}", *slot);
}

void notify_handler::handle_item_to_bank(packet_reader& reader) {
    auto slot = reader.read_u8();
    if (!slot || !game_) return;

    spdlog::info("Item stored in bank from slot {}", *slot);
}

void notify_handler::handle_cannot_item_to_bank(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Cannot store item in bank");
}

void notify_handler::handle_set_item_count(packet_reader& reader) {
    auto slot = reader.read_u8();
    auto count = reader.read_u32();

    if (!slot || !count || !game_) return;

    game_->inventory().set_item_count(*slot, *count);
}

void notify_handler::handle_item_lifespan_end(packet_reader& reader) {
    auto slot = reader.read_u8();
    if (!slot || !game_) return;

    game_->inventory().clear_slot(*slot);
    game_->show_message("Item has expired");
}

void notify_handler::handle_item_released(packet_reader& reader) {
    auto slot = reader.read_u8();
    if (!slot || !game_) return;

    spdlog::debug("Item released from slot {}", *slot);
}

void notify_handler::handle_item_color_change(packet_reader& reader) {
    auto slot = reader.read_u8();
    auto color = reader.read_u8();

    if (!slot || !color || !game_) return;

    game_->inventory().set_item_color(*slot, *color);
}

void notify_handler::handle_item_attribute_change(packet_reader& reader) {
    auto slot = reader.read_u8();
    auto attribute = reader.read_u32();

    if (!slot || !attribute || !game_) return;

    game_->inventory().set_item_attribute(*slot, *attribute);
}

void notify_handler::handle_cannot_carry_more_item(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Inventory full");
}

// ============================================================================
// SHOP/TRADE HANDLERS
// ============================================================================

void notify_handler::handle_item_purchased(packet_reader& reader) {
    auto slot = reader.read_u8();
    if (!slot || !game_) return;

    spdlog::info("Item purchased to slot {}", *slot);
}

void notify_handler::handle_not_enough_gold(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Not enough gold");
}

void notify_handler::handle_sell_item_price(packet_reader& reader) {
    auto price = reader.read_u32();
    if (!price || !game_) return;

    spdlog::debug("Sell price: {}", *price);
}

void notify_handler::handle_cannot_sell_item(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Cannot sell this item");
}

void notify_handler::handle_item_sold(packet_reader& reader) {
    auto slot = reader.read_u8();
    auto gold = reader.read_u32();

    if (!slot || !game_) return;

    game_->inventory().clear_slot(*slot);
    spdlog::info("Item sold for {} gold", gold.value_or(0));
}

void notify_handler::handle_repair_item_price(packet_reader& reader) {
    auto price = reader.read_u32();
    if (!price || !game_) return;

    spdlog::debug("Repair price: {}", *price);
}

void notify_handler::handle_cannot_repair_item(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Cannot repair this item");
}

void notify_handler::handle_item_repaired(packet_reader& reader) {
    auto slot = reader.read_u8();
    if (!slot || !game_) return;

    spdlog::info("Item in slot {} repaired", *slot);
}

void notify_handler::handle_reward_gold(packet_reader& reader) {
    auto gold = reader.read_u32();
    if (!gold || !game_) return;

    spdlog::info("Received {} gold", *gold);
}

// ============================================================================
// EXCHANGE HANDLERS
// ============================================================================

void notify_handler::handle_open_exchange_window(packet_reader& reader) {
    auto target_name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("Trade window opened with {}", target_name.value_or("Unknown"));

    // Update trade dialog with partner name
    if (auto* trade_dlg = dynamic_cast<trade_dialog*>(game_->ui().get_dialog(dialog_type::trade))) {
        trade_dlg->set_partner_name(target_name.value_or("Unknown"));
        trade_dlg->clear_my_items();
        trade_dlg->clear_partner_items();
        trade_dlg->set_my_confirmed(false);
        trade_dlg->set_partner_confirmed(false);
    }
    game_->ui().open_dialog(dialog_type::trade);
}

void notify_handler::handle_set_exchange_item(packet_reader& reader) {
    auto slot = reader.read_u8();
    auto item_id = reader.read_u16();

    if (!slot || !item_id || !game_) return;

    spdlog::debug("Exchange item set: slot {} item {}", *slot, *item_id);
}

void notify_handler::handle_cancel_exchange_item(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Trade cancelled");
    game_->ui().close_dialog(dialog_type::trade);
}

void notify_handler::handle_exchange_item_complete(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Trade completed");
    game_->ui().close_dialog(dialog_type::trade);
    game_->show_message("Trade completed!");
}

// ============================================================================
// GUILD HANDLERS
// ============================================================================

void notify_handler::handle_guild_disbanded(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Guild has been disbanded");
    game_->show_message("Guild disbanded");
}

void notify_handler::handle_new_guildsman(packet_reader& reader) {
    auto name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("New guild member: {}", name.value_or("Unknown"));
}

void notify_handler::handle_dismiss_guildsman(packet_reader& reader) {
    auto name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("Guild member dismissed: {}", name.value_or("Unknown"));
}

void notify_handler::handle_query_join_guild_permission(packet_reader& reader) {
    auto name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("{} wants to join the guild", name.value_or("Unknown"));
}

void notify_handler::handle_query_dismiss_guild_permission(packet_reader& reader) {
    auto name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("Request to dismiss {}", name.value_or("Unknown"));
}

void notify_handler::handle_wait_for_guild_operation(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_message("Waiting for guild master approval...");
}

void notify_handler::handle_cannot_join_more_guildsman(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Guild is full");
}

// ============================================================================
// PARTY HANDLERS
// ============================================================================

void notify_handler::handle_party(packet_reader& reader) {
    auto count = reader.read_u8();
    if (!count || !game_) return;

    spdlog::debug("Party update: {} members", *count);
}

void notify_handler::handle_response_create_new_party(packet_reader& reader) {
    auto result = reader.read_u8();
    if (!result || !game_) return;

    if (*result == 1) {
        spdlog::info("Party created");
    } else {
        game_->show_error("Failed to create party");
    }
}

void notify_handler::handle_query_join_party(packet_reader& reader) {
    auto name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("{} wants to join your party", name.value_or("Unknown"));
}

// ============================================================================
// QUEST HANDLERS
// ============================================================================

void notify_handler::handle_quest_contents(packet_reader& reader) {
    auto quest_id = reader.read_u16();
    if (!quest_id || !game_) return;

    spdlog::debug("Quest {} contents received", *quest_id);
}

void notify_handler::handle_quest_completed(packet_reader& reader) {
    auto quest_id = reader.read_u16();
    if (!quest_id || !game_) return;

    spdlog::info("Quest {} completed!", *quest_id);
    game_->show_message("Quest completed!");
}

void notify_handler::handle_quest_reward(packet_reader& reader) {
    auto reward_type = reader.read_u8();
    auto amount = reader.read_u32();

    if (!reward_type || !game_) return;

    spdlog::info("Quest reward: type {} amount {}", *reward_type, amount.value_or(0));
}

// ============================================================================
// CRAFTING HANDLERS
// ============================================================================

void notify_handler::handle_build_item_success(packet_reader& reader) {
    auto item_name = reader.read_string(32);
    if (!game_) return;

    spdlog::info("Crafted: {}", item_name.value_or("Unknown"));
    game_->show_message("Item crafted!");
}

void notify_handler::handle_build_item_fail(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Crafting failed");
}

void notify_handler::handle_portion_success(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_message("Potion created!");
}

void notify_handler::handle_item_upgrade_fail(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    game_->show_error("Upgrade failed");
}

// ============================================================================
// FISHING HANDLERS
// ============================================================================

void notify_handler::handle_fish_chance(packet_reader& reader) {
    auto chance = reader.read_u8();
    if (!chance || !game_) return;

    spdlog::debug("Fishing chance: {}%", *chance);
}

void notify_handler::handle_fish_success(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Caught a fish!");
}

void notify_handler::handle_fish_fail(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::debug("Fish got away");
}

// ============================================================================
// PK/COMBAT HANDLERS
// ============================================================================

void notify_handler::handle_pk_penalty(packet_reader& reader) {
    auto penalty = reader.read_u8();
    if (!penalty || !game_) return;

    spdlog::warn("PK penalty: {}", *penalty);
}

void notify_handler::handle_pk_captured(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("PK captured!");
}

void notify_handler::handle_enemy_kill_reward(packet_reader& reader) {
    auto reward = reader.read_u32();
    if (!reward || !game_) return;

    spdlog::info("Enemy kill reward: {}", *reward);
}

void notify_handler::handle_enemy_kills(packet_reader& reader) {
    auto kills = reader.read_u32();
    if (!kills || !game_) return;

    spdlog::debug("Enemy kills: {}", *kills);
}

// ============================================================================
// CRUSADE HANDLERS
// ============================================================================

void notify_handler::handle_crusade(packet_reader& reader) {
    auto status = reader.read_u8();
    if (!status || !game_) return;

    spdlog::info("Crusade status: {}", *status);
}

void notify_handler::handle_locked_map(packet_reader& reader) {
    auto map_name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("Map {} is locked", map_name.value_or("Unknown"));
}

void notify_handler::handle_duty_selected(packet_reader& reader) {
    auto duty = reader.read_u8();
    if (!duty || !game_) return;

    spdlog::info("Duty selected: {}", *duty);
}

void notify_handler::handle_map_status_next(packet_reader& reader) {
    (void)reader;
    spdlog::debug("Map status next");
}

void notify_handler::handle_map_status_last(packet_reader& reader) {
    (void)reader;
    spdlog::debug("Map status last");
}

void notify_handler::handle_meteor_strike_coming(packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!x || !y || !game_) return;

    spdlog::warn("Meteor strike incoming at ({}, {})", *x, *y);
}

void notify_handler::handle_meteor_strike_hit(packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!x || !y || !game_) return;

    spdlog::info("Meteor strike hit at ({}, {})", *x, *y);
}

void notify_handler::handle_grand_magic_result(packet_reader& reader) {
    auto result = reader.read_u8();
    if (!result || !game_) return;

    spdlog::info("Grand magic result: {}", *result);
}

// ============================================================================
// SPECIAL ABILITY HANDLERS
// ============================================================================

void notify_handler::handle_energy_sphere_created(packet_reader& reader) {
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!x || !y || !game_) return;

    spdlog::info("Energy sphere created at ({}, {})", *x, *y);
}

void notify_handler::handle_energy_sphere_goal_in(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Energy sphere goal achieved!");
}

void notify_handler::handle_special_ability_enabled(packet_reader& reader) {
    auto ability = reader.read_u8();
    if (!ability || !game_) return;

    spdlog::info("Special ability {} enabled", *ability);
}

void notify_handler::handle_special_ability_status(packet_reader& reader) {
    auto ability = reader.read_u8();
    auto status = reader.read_u8();

    if (!ability || !status || !game_) return;

    spdlog::debug("Special ability {} status: {}", *ability, *status);
}

// ============================================================================
// WORLD EVENT HANDLERS
// ============================================================================

void notify_handler::handle_new_dynamic_object(packet_reader& reader) {
    auto obj_type = reader.read_u16();
    auto x = reader.read_i16();
    auto y = reader.read_i16();

    if (!obj_type || !x || !y || !game_) return;

    spdlog::debug("New dynamic object type {} at ({}, {})", *obj_type, *x, *y);
}

void notify_handler::handle_del_dynamic_object(packet_reader& reader) {
    auto obj_id = reader.read_u32();
    if (!obj_id || !game_) return;

    spdlog::debug("Dynamic object {} deleted", *obj_id);
}

void notify_handler::handle_weather_change(packet_reader& reader) {
    auto weather = reader.read_u8();
    if (!weather || !game_) return;

    if (*weather <= static_cast<uint8_t>(weather_type::heavy_snow))
    {
        game_->game_world().set_weather(static_cast<weather_type>(*weather));
    }
    else
    {
        spdlog::warn("Invalid weather type: {}", *weather);
    }
}

void notify_handler::handle_time_change(packet_reader& reader) {
    auto hour = reader.read_u8();
    if (!hour || !game_) return;

    if (*hour <= static_cast<uint8_t>(time_of_day::midnight))
    {
        game_->game_world().set_time(static_cast<time_of_day>(*hour));
    }
    else
    {
        spdlog::warn("Invalid time of day: {}", *hour);
    }
}

// ============================================================================
// SYSTEM HANDLERS
// ============================================================================

void notify_handler::handle_event_msg_string(packet_reader& reader) {
    auto msg = reader.read_string(256);
    if (!msg || !game_) return;

    spdlog::info("Event message: {}", *msg);
    game_->show_message(*msg);
}

void notify_handler::handle_server_shutdown(packet_reader& reader) {
    auto minutes = reader.read_u8();
    if (!game_) return;

    spdlog::warn("Server shutting down in {} minutes", minutes.value_or(0));
    game_->show_message("Server shutting down soon!");
}

void notify_handler::handle_total_users(packet_reader& reader) {
    auto count = reader.read_u32();
    if (!count || !game_) return;

    spdlog::debug("Total users online: {}", *count);
}

void notify_handler::handle_force_disconn(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::warn("Forced disconnect");
    game_->change_state(game_state::connection_lost);
}

// ============================================================================
// MISC HANDLERS
// ============================================================================

void notify_handler::handle_npc_talk(packet_reader& reader) {
    auto npc_name = reader.read_string(20);
    auto message = reader.read_string(256);

    if (!game_) return;

    spdlog::debug("NPC {}: {}", npc_name.value_or("NPC"), message.value_or(""));
}

void notify_handler::handle_player_profile(packet_reader& reader) {
    auto name = reader.read_string(20);
    auto guild = reader.read_string(20);
    auto level = reader.read_u16();

    if (!game_) return;

    spdlog::info("Player profile: {} (Guild: {}, Level: {})",
        name.value_or("Unknown"), guild.value_or("None"), level.value_or(0));
}

void notify_handler::handle_player_on_game(packet_reader& reader) {
    auto name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("{} is online", name.value_or("Unknown"));
}

void notify_handler::handle_player_not_on_game(packet_reader& reader) {
    auto name = reader.read_string(20);
    if (!game_) return;

    spdlog::info("{} is offline", name.value_or("Unknown"));
}

void notify_handler::handle_whisper_mode_on(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Whisper mode enabled");
}

void notify_handler::handle_whisper_mode_off(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Whisper mode disabled");
}

void notify_handler::handle_player_shutup(packet_reader& reader) {
    auto duration = reader.read_u16();
    if (!game_) return;

    spdlog::warn("Muted for {} seconds", duration.value_or(0));
}

void notify_handler::handle_to_be_recalled(packet_reader& reader) {
    (void)reader;
    if (!game_) return;

    spdlog::info("Being recalled...");
}

void notify_handler::handle_show_map(packet_reader& reader) {
    auto map_id = reader.read_u8();
    if (!map_id || !game_) return;

    spdlog::debug("Show map: {}", *map_id);
}

void notify_handler::handle_server_change(packet_reader& reader) {
    auto server_name = reader.read_string(32);
    if (!game_) return;

    spdlog::info("Server change: {}", server_name.value_or("Unknown"));
}

void notify_handler::handle_observer_mode(packet_reader& reader) {
    auto mode = reader.read_u8();
    if (!mode || !game_) return;

    spdlog::info("Observer mode: {}", *mode ? "ON" : "OFF");
}

void notify_handler::handle_limited_level(packet_reader& reader) {
    auto level = reader.read_u16();
    if (!level || !game_) return;

    spdlog::debug("Limited level: {}", *level);
}

} // namespace hb
