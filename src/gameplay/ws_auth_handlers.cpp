#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "ui/dialogs/skills_dialog.hpp"
#include "ui/dialogs/spellbook_dialog.hpp"
#include "world/tile.hpp"
#include <spdlog/spdlog.h>

namespace hb
{

namespace
{

time_of_day hour_to_time_of_day(uint8_t hour)
{
    if (hour < 5)
        return time_of_day::midnight;
    if (hour < 7)
        return time_of_day::dawn;
    if (hour < 10)
        return time_of_day::morning;
    if (hour < 14)
        return time_of_day::noon;
    if (hour < 17)
        return time_of_day::afternoon;
    if (hour < 19)
        return time_of_day::dusk;
    if (hour < 23)
        return time_of_day::night;
    return time_of_day::midnight;
}

} // anonymous namespace

void ws_message_handler::handle_login_response_ws(const json& message)
{
    auto response = login_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info("Login successful! Session: {}", response.session_token);
        session_token_ = response.session_token;

        pending_username_.clear();
        pending_password_.clear();

        game_->ui().show_connection_dialog(nullptr);
        request_characters();
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Login failed" : response.error_message;
        spdlog::warn("Login failed: {}", error_msg);
        game_->show_error(error_msg);

        pending_username_.clear();
        pending_password_.clear();
    }
}

void ws_message_handler::handle_get_characters_response(const json& message)
{
    auto response = get_characters_response_data::from_json(message);

    game_->ui().hide_connection_dialog();

    if (response.success)
    {
        std::vector<character_info> chars;
        for (const auto& sc : response.characters)
        {
            character_info info;
            info.id = sc.id;
            info.name = sc.name;
            info.level = static_cast<uint16_t>(sc.level);
            info.warrior = (sc.class_type == 0);
            info.gender = static_cast<uint8_t>(sc.gender == 0 ? 1 : 2);
            info.skin_color = static_cast<uint8_t>(sc.skin_color);
            info.hair_style = static_cast<uint8_t>(sc.hair_style);
            info.hair_color = static_cast<uint8_t>(sc.hair_color);
            info.underwear_color = static_cast<uint8_t>(sc.underwear_color);
            info.body_armor = sc.body_armor;
            info.arm_armor = sc.arm_armor;
            info.pants = sc.pants;
            info.boots = sc.boots;
            info.helmet = sc.helmet;
            info.mantle = sc.mantle;
            info.weapon = sc.weapon;
            info.shield = sc.shield;
            chars.push_back(info);
        }

        game_->set_characters(std::move(chars));
        spdlog::info("Received {} characters", game_->characters().size());
        game_->change_state(game_state::select_character);
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Failed to get characters" : response.error_message;
        spdlog::warn("Get characters failed: {}", error_msg);
        game_->show_error(error_msg);
    }
}

void ws_message_handler::handle_enter_game_response(const json& message)
{
    auto response = enter_game_response_data::from_json(message);

    game_->ui().hide_connection_dialog();

    if (!response.success)
    {
        std::string error_msg = response.error_message.empty() ? "Failed to enter game" : response.error_message;
        spdlog::warn("Enter game failed: {}", error_msg);

        if (response.error_message == "account_already_in_game")
        {
            int32_t char_id = pending_enter_game_character_id_;
            game_->ui().create_confirm_box("Session Conflict",
                                           "This account is already logged in.\nDisconnect other session?",
                                           [this, char_id](bool confirmed)
                                           {
                                               if (confirmed)
                                               {
                                                   request_enter_game(char_id, true);
                                               }
                                           });
            return;
        }

        game_->show_error(error_msg);
        return;
    }

    const auto& ch = response.character;
    spdlog::info("Entering game as '{}' on map: {} at ({}, {})", ch.name, ch.map_name, ch.pos_x, ch.pos_y);

    auto& entities = game_->entities();
    auto& sprites = game_->sprites();

    // Create local player entity
    auto& player = entities.create_entity_with_id(ch.id, entity_type::player);
    entities.set_local_player(ch.id);

    auto& transform = player.transform();
    transform.tile_x = ch.pos_x;
    transform.tile_y = ch.pos_y;
    transform.move_start_x = ch.pos_x;
    transform.move_start_y = ch.pos_y;
    transform.x = ch.pos_x * 32 + 16;
    transform.y = ch.pos_y * 32 + 16;
    spdlog::info("PLAYER SPAWN: tile=({},{}) world=({},{})", ch.pos_x, ch.pos_y, transform.x, transform.y);

    auto& name = player.name();
    name.name = ch.name;
    name.faction = ch.nation == 1 ? "aresden" : ch.nation == 2 ? "elvine" : "neutral";
    name.guild_name = ch.guild_name;
    name.guild_tag = ch.guild_tag;
    name.guild_rank_id = ch.guild_rank;

    // Initialize guild system from enter_game data
    if (!ch.guild_name.empty())
    {
        game_->guild().set_guild(ch.guild_name, ch.guild_tag, ch.guild_rank);
    }

    auto& stats = player.stats();
    stats.level = static_cast<uint16_t>(ch.level);
    stats.strength = static_cast<uint16_t>(ch.str);
    stats.vitality = static_cast<uint16_t>(ch.vit);
    stats.dexterity = static_cast<uint16_t>(ch.dex);
    stats.intelligence = static_cast<uint16_t>(ch.int_);
    stats.magic = static_cast<uint16_t>(ch.mag);
    stats.charisma = static_cast<uint16_t>(ch.cha);
    stats.hp = ch.hp;
    stats.max_hp = ch.hp_max;
    stats.mp = ch.mp;
    stats.max_mp = ch.mp_max;
    stats.sp = ch.sp;
    stats.max_sp = ch.sp_max;
    stats.experience = ch.experience;
    stats.hunger = static_cast<uint8_t>(ch.hunger_level);

    auto& combat = player.combat();
    combat.pk_count = ch.pk_count;

    auto& sprite = player.sprite();
    sprite.skin_color = static_cast<uint8_t>(ch.skin_color);
    sprite.hair_style = static_cast<uint8_t>(ch.hair_style);
    sprite.hair_color = static_cast<uint8_t>(ch.hair_color);
    sprite.underwear_color = static_cast<uint8_t>(ch.underwear_color);
    sprite.gender = (ch.gender == 0) ? 1 : 2;

    entities.load_character_sprites(player, sprites);

    auto& anim = player.animation();
    anim.set_state(entity_anim_state::stop);

    spdlog::debug("Player stats - Level: {}, STR: {}, DEX: {}, VIT: {}, INT: {}, MAG: {}, CHA: {}",
                  stats.level,
                  stats.strength,
                  stats.dexterity,
                  stats.vitality,
                  stats.intelligence,
                  stats.magic,
                  stats.charisma);
    spdlog::debug("Player vitals - HP: {}/{}, MP: {}/{}, SP: {}/{}",
                  stats.hp,
                  stats.max_hp,
                  stats.mp,
                  stats.max_mp,
                  stats.sp,
                  stats.max_sp);

    // Populate inventory (equipment is unified — equipped items have equipped_slot set)
    auto& inventory = game_->inventory();
    inventory.clear();
    inventory.set_gold(static_cast<uint32_t>(ch.gold));

    // Clear all equipped slots first
    for (int i = 1; i <= 12; ++i)
        inventory.clear_equipped(static_cast<equip_slot>(i));

    for (const auto& inv_item : response.inventory.items)
    {
        item itm;
        itm.id = inv_item.item_id;
        itm.template_id = inv_item.template_id;
        itm.name = inv_item.name;
        itm.amount = static_cast<uint32_t>(inv_item.count);
        itm.durability = static_cast<uint16_t>(inv_item.durability);
        itm.max_durability = static_cast<uint16_t>(inv_item.max_durability);
        itm.type = static_cast<item_type>(inv_item.item_type);
        itm.slot = equip_slot_from_server(inv_item.equip_pos);
        itm.sprite_id = static_cast<uint16_t>(inv_item.sprite);
        itm.equipped_sprite_id = static_cast<uint16_t>(inv_item.sprite_frame);
        itm.color = static_cast<uint8_t>(inv_item.color);
        itm.weight = static_cast<uint32_t>(inv_item.weight);
        itm.level_req = static_cast<uint16_t>(inv_item.level_limit);
        itm.attribute = inv_item.attribute;
        inventory.set_item_at(inv_item.slot, itm);

        if (inv_item.pos_x != 0 || inv_item.pos_y != 0)
            inventory.set_slot_position(inv_item.slot, inv_item.pos_x, inv_item.pos_y);

        // Equipped items: inventory items with equipped_slot set
        if (inv_item.equipped_slot >= 0)
        {
            auto eq_slot = equip_slot_from_server(inv_item.equipped_slot);
            if (eq_slot != equip_slot::none)
                inventory.set_equipped(eq_slot, itm);
        }
    }
    spdlog::debug("Loaded {} inventory items, {} gold", response.inventory.items.size(), ch.gold);

    // Set skill levels
    auto& skills = game_->skills();
    spdlog::info("enter_game_response contains {} skills", response.skills.size());
    for (const auto& sk : response.skills)
    {
        uint8_t mastery = static_cast<uint8_t>(std::min(static_cast<int16_t>(100), sk.level));
        skills.set_mastery(sk.skill_id, mastery);

        float progress = 0.0f;
        if (sk.uses_to_next_level > 0)
            progress = static_cast<float>(sk.uses_this_level) / static_cast<float>(sk.uses_to_next_level);
        skills.set_sub_progress(sk.skill_id, progress);

        spdlog::info("  enter_game skill: id={} level={} mastery={} uses={}/{} progress={:.2f}",
                     sk.skill_id,
                     sk.level,
                     mastery,
                     sk.uses_this_level,
                     sk.uses_to_next_level,
                     progress);
    }

    // Populate skills dialog
    if (auto* skill_dlg = dynamic_cast<skills_dialog*>(game_->ui().get_dialog(dialog_type::skills)))
    {
        auto all = skills.get_all_skills();
        std::vector<skill> skill_list;
        skill_list.reserve(all.size());
        for (const auto* s : all)
            skill_list.push_back(*s);
        skill_dlg->set_skills(skill_list);
        spdlog::info("Pushed {} skills to dialog from enter_game", skill_list.size());
    }
    else
    {
        spdlog::warn("Skills dialog not found during enter_game!");
    }

    // Set known spells from server
    auto& magic = game_->magic();
    for (const auto& sp : response.spells)
    {
        magic.learn_spell(sp.spell_id);
        magic.set_spell_mastery(sp.spell_id, static_cast<uint8_t>(sp.level));
        magic.set_spell_total_casts(sp.spell_id, sp.total_casts);
    }
    spdlog::info(
        "Loaded {} spells from server, {} learned total", response.spells.size(), magic.get_learned_spells().size());

    // Auto-populate spell hotbar from learned spells
    game_->auto_populate_spell_hotbar();

    // Populate spellbook dialog with all spell definitions
    if (auto* spell_dlg = dynamic_cast<spellbook_dialog*>(game_->ui().get_dialog(dialog_type::spellbook)))
    {
        spell_dlg->clear_spells();
        auto all_spells = magic.get_all_spells();
        for (const auto* sp : all_spells)
        {
            spell_dlg->add_spell(*sp);
        }
        spdlog::info("Populated spellbook dialog with {} spells ({} learned)",
                     all_spells.size(),
                     magic.get_learned_spells().size());
    }
    else
    {
        spdlog::warn("Spellbook dialog not found!");
    }

    // Load quest data
    auto& quests = game_->quests();
    quests.clear();
    for (const auto& aq : response.quests.active)
    {
        active_quest quest;
        quest.quest_id = aq.quest_id;
        quest.status = aq.status;
        for (const auto& obj : aq.objectives)
        {
            quest.objectives.push_back({obj.id, obj.status, obj.current, obj.required});
        }
        quests.active.push_back(std::move(quest));
    }
    quests.completed = response.quests.completed;
    spdlog::debug("Loaded {} active quests, {} completed quests", quests.active.size(), quests.completed.size());

    // Nearby entities are now sent as individual entity_spawn/npc_spawn/ground_item_spawn
    // messages by the server, so no entity list parsing needed here.

    // Load map
    auto& world = game_->game_world();
    if (!world.load_map(ch.map_name))
    {
        spdlog::warn("Failed to load map data for '{}', continuing without tile data", ch.map_name);
    }

    // Apply initial environment state from enter_game_response
    {
        uint8_t hour = response.world.time_hour;
        uint8_t minute = response.world.time_minute;
        uint8_t weather = response.world.weather;

        world.set_clock(hour, minute);
        world.set_time(hour_to_time_of_day(hour));

        if (weather <= static_cast<uint8_t>(weather_type::heavy_snow))
            world.set_weather(static_cast<weather_type>(weather));

        spdlog::info("Initial environment: {}:{:02d}, weather={}", hour, minute, weather);
    }

    int32_t player_world_x = ch.pos_x * 32 + 16;
    int32_t player_world_y = ch.pos_y * 32 + 16;
    world.set_player_position(player_world_x, player_world_y);
    spdlog::debug(
        "Player position set at tile ({},{}) -> world ({},{})", ch.pos_x, ch.pos_y, player_world_x, player_world_y);

    spdlog::info("Entering game world: {}", ch.map_name);

    send_view_range();
    send_chat_preferences();
    game_->change_state(game_state::playing);
}

void ws_message_handler::handle_create_character_response(const json& message)
{
    auto response = create_character_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info("Character created successfully! ID: {}", response.character_id);
        request_characters();
        game_->change_state(game_state::select_character);
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Failed to create character" : response.error_message;
        spdlog::warn("Character creation failed: {}", error_msg);
        game_->show_error(error_msg);
    }
}

void ws_message_handler::handle_delete_character_response(const json& message)
{
    auto response = delete_character_response_data::from_json(message);

    if (response.success)
    {
        spdlog::info("Character deleted successfully");
        // Server sends an unsolicited get_characters_response with the updated list,
        // so we don't need to request_characters() here.
    }
    else
    {
        std::string error_msg = response.error_message.empty() ? "Failed to delete character" : response.error_message;
        spdlog::warn("Character deletion failed: {}", error_msg);
        game_->show_error(error_msg);
    }
}

void ws_message_handler::request_characters()
{
    spdlog::info("Requesting character list");
    json msg = make_get_characters_request();
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_enter_game(int32_t character_id, bool force_disconnect)
{
    spdlog::info("Requesting to enter game with character ID: {}{}",
                 character_id,
                 force_disconnect ? " (force disconnect)" : "");

    pending_enter_game_character_id_ = character_id;

    game_->ui().show_connection_dialog(nullptr);

    json msg = make_enter_game_request(character_id, force_disconnect);
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_create_character(const character_create_data& data)
{
    spdlog::info("Requesting to create character: {}", data.name);

    json msg;
    msg["type"] = "create_character_request";
    msg["data"] = {{"name", data.name},
                   {"gender", data.gender},
                   {"skin_color", data.skin_color},
                   {"hair_style", data.hair_style},
                   {"hair_color", data.hair_color},
                   {"underwear_color", data.underwear_color},
                   {"strength", data.strength},
                   {"vitality", data.vitality},
                   {"dexterity", data.dexterity},
                   {"intelligence", data.intelligence},
                   {"magic", data.magic},
                   {"charisma", data.charisma}};
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_delete_character(int32_t character_id)
{
    spdlog::info("Requesting to delete character ID: {}", character_id);

    json msg;
    msg["type"] = "delete_character_request";
    msg["data"] = {{"character_id", character_id}};
    game_->ws_connection().send(msg);
}

void ws_message_handler::request_entity_info(uint32_t entity_id)
{
    spdlog::debug("Requesting entity info for entity {}", entity_id);
    json msg = make_entity_info_request(entity_id);
    game_->ws_connection().send(msg);
}

} // namespace hb
