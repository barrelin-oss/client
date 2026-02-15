#pragma once

#include "network/packet.hpp"
#include "network/protocol.hpp"
#include <functional>
#include <unordered_map>

namespace hb
{

// Forward declarations
class game_state_manager;

// Notification handler function type
using notify_handler_fn = std::function<void(game_state_manager&, packet_reader&)>;

// Notification handler dispatcher
class notify_handler
{
public:
    notify_handler() = default;

    // Initialize with game state reference
    void initialize(game_state_manager& game);

    // Dispatch a notification by subtype
    void dispatch(uint16_t subtype, packet_reader& reader);

private:
    // Register all handlers
    void register_handlers();

    // Individual handlers
    void handle_hp(packet_reader& reader);
    void handle_mp(packet_reader& reader);
    void handle_sp(packet_reader& reader);
    void handle_exp(packet_reader& reader);
    void handle_level_up(packet_reader& reader);
    void handle_item_obtained(packet_reader& reader);
    void handle_item_drop(packet_reader& reader);
    void handle_killed(packet_reader& reader);
    void handle_magic_effect_on(packet_reader& reader);
    void handle_magic_effect_off(packet_reader& reader);
    void handle_skill(packet_reader& reader);
    void handle_magic_study_success(packet_reader& reader);
    void handle_magic_study_fail(packet_reader& reader);
    void handle_skill_train_success(packet_reader& reader);
    void handle_skill_train_fail(packet_reader& reader);
    void handle_charisma(packet_reader& reader);
    void handle_hunger(packet_reader& reader);
    void handle_safe_attack_mode(packet_reader& reader);
    void handle_super_attack_left(packet_reader& reader);
    void handle_event_msg_string(packet_reader& reader);
    void handle_new_dynamic_object(packet_reader& reader);
    void handle_del_dynamic_object(packet_reader& reader);
    void handle_weather_change(packet_reader& reader);
    void handle_time_change(packet_reader& reader);
    void handle_server_shutdown(packet_reader& reader);
    void handle_total_users(packet_reader& reader);
    void handle_force_disconn(packet_reader& reader);
    void handle_damage_move(packet_reader& reader);

    // Item handlers
    void handle_item_to_bank(packet_reader& reader);
    void handle_cannot_item_to_bank(packet_reader& reader);
    void handle_set_item_count(packet_reader& reader);
    void handle_item_lifespan_end(packet_reader& reader);
    void handle_item_released(packet_reader& reader);
    void handle_item_color_change(packet_reader& reader);
    void handle_item_attribute_change(packet_reader& reader);

    // Shop/Trade handlers
    void handle_sell_item_price(packet_reader& reader);
    void handle_cannot_sell_item(packet_reader& reader);
    void handle_item_sold(packet_reader& reader);
    void handle_repair_item_price(packet_reader& reader);
    void handle_cannot_repair_item(packet_reader& reader);
    void handle_item_repaired(packet_reader& reader);
    void handle_item_purchased(packet_reader& reader);
    void handle_not_enough_gold(packet_reader& reader);
    void handle_cannot_carry_more_item(packet_reader& reader);
    void handle_reward_gold(packet_reader& reader);

    // Exchange handlers
    void handle_open_exchange_window(packet_reader& reader);
    void handle_set_exchange_item(packet_reader& reader);
    void handle_cancel_exchange_item(packet_reader& reader);
    void handle_exchange_item_complete(packet_reader& reader);

    // Guild handlers
    void handle_guild_disbanded(packet_reader& reader);
    void handle_new_guildsman(packet_reader& reader);
    void handle_dismiss_guildsman(packet_reader& reader);
    void handle_query_join_guild_permission(packet_reader& reader);
    void handle_query_dismiss_guild_permission(packet_reader& reader);
    void handle_wait_for_guild_operation(packet_reader& reader);
    void handle_cannot_join_more_guildsman(packet_reader& reader);

    // Party handlers
    void handle_party(packet_reader& reader);
    void handle_response_create_new_party(packet_reader& reader);
    void handle_query_join_party(packet_reader& reader);

    // Quest handlers
    void handle_quest_contents(packet_reader& reader);
    void handle_quest_completed(packet_reader& reader);
    void handle_quest_reward(packet_reader& reader);

    // Crafting handlers
    void handle_build_item_success(packet_reader& reader);
    void handle_build_item_fail(packet_reader& reader);
    void handle_portion_success(packet_reader& reader);
    void handle_item_upgrade_fail(packet_reader& reader);

    // Fishing handlers
    void handle_fish_chance(packet_reader& reader);
    void handle_fish_success(packet_reader& reader);
    void handle_fish_fail(packet_reader& reader);

    // Combat/PK handlers
    void handle_pk_penalty(packet_reader& reader);
    void handle_pk_captured(packet_reader& reader);
    void handle_enemy_kill_reward(packet_reader& reader);
    void handle_enemy_kills(packet_reader& reader);

    // Crusade handlers
    void handle_crusade(packet_reader& reader);
    void handle_locked_map(packet_reader& reader);
    void handle_duty_selected(packet_reader& reader);
    void handle_map_status_next(packet_reader& reader);
    void handle_map_status_last(packet_reader& reader);
    void handle_meteor_strike_coming(packet_reader& reader);
    void handle_meteor_strike_hit(packet_reader& reader);
    void handle_grand_magic_result(packet_reader& reader);

    // Special ability handlers
    void handle_energy_sphere_created(packet_reader& reader);
    void handle_energy_sphere_goal_in(packet_reader& reader);
    void handle_special_ability_enabled(packet_reader& reader);
    void handle_special_ability_status(packet_reader& reader);

    // Misc handlers
    void handle_npc_talk(packet_reader& reader);
    void handle_player_profile(packet_reader& reader);
    void handle_player_on_game(packet_reader& reader);
    void handle_player_not_on_game(packet_reader& reader);
    void handle_whisper_mode_on(packet_reader& reader);
    void handle_whisper_mode_off(packet_reader& reader);
    void handle_player_shutup(packet_reader& reader);
    void handle_to_be_recalled(packet_reader& reader);
    void handle_show_map(packet_reader& reader);
    void handle_server_change(packet_reader& reader);
    void handle_observer_mode(packet_reader& reader);
    void handle_global_attack_mode(packet_reader& reader);
    void handle_limited_level(packet_reader& reader);

    game_state_manager* game_ = nullptr;
    std::unordered_map<uint16_t, notify_handler_fn> handlers_;
};

} // namespace hb
