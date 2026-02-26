#pragma once

#include "core/game_enums.hpp"
#include "graphics/text_style.hpp"
#include "audio/sound_types.hpp"
#include "assets/sprite.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>

namespace hb
{

// Transform component - position and movement
// tile_x/y is the logical position (set to destination IMMEDIATELY when movement starts).
// move_start_x/y stores the origin tile for rendering interpolation.
// Interpolation goes from move_start → tile using move_progress (0→1).
struct transform_component
{
    int32_t x = 0;      // World X position (interpolated during movement)
    int32_t y = 0;      // World Y position (interpolated during movement)
    int32_t tile_x = 0; // Logical tile position (destination during movement)
    int32_t tile_y = 0;
    direction facing = direction::south;
    float move_progress = 0.0f; // 0.0 to 1.0 for smooth movement
    int32_t move_start_x = 0;   // Origin tile when movement started
    int32_t move_start_y = 0;
    bool moving = false;
};

// Sprite component - visual representation
struct sprite_component
{
    const sprite* body_sprite = nullptr;
    const sprite* armor_sprite = nullptr;
    const sprite* weapon_sprite = nullptr;
    const sprite* shield_sprite = nullptr;
    const sprite* helm_sprite = nullptr;
    const sprite* hair_sprite = nullptr;
    const sprite* underwear_sprite = nullptr;
    const sprite* effect_sprite = nullptr;

    // Equipment type IDs (0 = not equipped, used for per-frame sprite lookup)
    uint8_t body_armor = 0;   // 1-15
    uint8_t arm_armor = 0;    // 1-15
    uint8_t pants = 0;        // 1-15 (1=skirt for female)
    uint8_t boots = 0;        // 1-15
    uint8_t helmet = 0;       // 1-15
    uint8_t mantle = 0;       // 1-15
    uint8_t weapon = 0;       // 0-255 (wider range than armor)
    uint8_t shield = 0;       // 1-9

    uint8_t body_frame = 0;
    uint8_t armor_frame = 0;
    uint8_t weapon_frame = 0;

    uint8_t skin_color = 0;
    uint8_t hair_style = 0;
    uint8_t hair_color = 0;
    uint8_t underwear_color = 0;
    uint8_t gender = 1; // 1 = male, 2 = female

    float alpha = 1.0f; // For transparency effects
    bool visible = true;
    bool flipped = false; // Horizontal flip based on direction
};

// Animation states (matching original DrawObject_OnXXX handlers)
enum class entity_anim_state : uint8_t
{
    stop = 0,          // DrawObject_OnStop - standing idle
    move = 1,          // DrawObject_OnMove - walking
    run = 2,           // DrawObject_OnRun - running
    attack = 3,        // DrawObject_OnAttack - melee attack
    attack_move = 4,   // DrawObject_OnAttackMove - attack while moving
    damage = 5,        // DrawObject_OnDamage - taking damage
    damage_move = 6,   // DrawObject_OnDamageMove - hit while moving
    magic = 7,         // DrawObject_OnMagic - casting spell
    get_item = 8,      // DrawObject_OnGetItem - picking up item
    dying = 9,         // DrawObject_OnDying - death animation
    dead = 10,         // DrawObject_OnDead - corpse on ground
    magic_attack = 11, // DrawObject_OnMagicAttack - spell hit
};

// Animation component - frame animation
struct animation_component
{
    // Legacy anim_type for compatibility
    enum class anim_type : uint8_t
    {
        idle = 0,
        walk = 1,
        run = 2,
        attack = 3,
        attack_spell = 4,
        damage = 5,
        dying = 6,
        dead = 7,
        special = 8,
    };

    anim_type current_anim = anim_type::idle;
    entity_anim_state state = entity_anim_state::stop;
    uint8_t current_frame = 0;
    uint8_t frame_count = 8; // Default to idle: 8 frames
    float frame_timer = 0.0f;
    float frame_duration = 0.115f; // Default to idle: 115ms per frame
    bool looping = true;
    bool finished = false;
    // Animation callbacks
    uint8_t attack_frame = 0; // Frame when attack damage is dealt
    bool attack_triggered = false;

    // Pending weapon impact sound — stored when damage starts, played at frame 4
    // (the composite split point where idle prefix ends and damage sprite begins).
    // Legacy: MapData.cpp plays C5/C6/C7 at frame 4 of DEF_OBJECTDAMAGE/DEF_OBJECTDYING.
    std::optional<character_sound> pending_impact_sound;

    // Frame data per state (matching legacy Helbreath - maxFrame=N means N+1 frames: 0 to N)
    static constexpr uint8_t frames_idle = 8;         // Legacy DEF_OBJECTSTOP maxFrame=7 → 8 frames
    static constexpr uint8_t frames_walk = 8;         // Legacy DEF_OBJECTMOVE maxFrame=7 → 8 frames
    static constexpr uint8_t frames_run = 8;          // Legacy DEF_OBJECTRUN maxFrame=7 → 8 frames
    static constexpr uint8_t frames_attack = 8;       // Legacy DEF_OBJECTATTACK maxFrame=7 → 8 frames
    static constexpr uint8_t frames_attack_move = 13; // Legacy DEF_OBJECTATTACKMOVE maxFrame=12 → 13 frames
    static constexpr uint8_t frames_damage = 8;       // Legacy DEF_OBJECTDAMAGE maxFrame=7 → 8 frames
    static constexpr uint8_t frames_damage_move = 4;  // Legacy DEF_OBJECTDAMAGEMOVE maxFrame=3 → 4 frames
    static constexpr uint8_t frames_dying = 13;       // Legacy DEF_OBJECTDYING maxFrame=12 → 13 frames
    static constexpr uint8_t frames_dead = 1;
    static constexpr uint8_t frames_magic = 16;   // Legacy DEF_OBJECTMAGIC maxFrame=15 → 16 frames
    static constexpr uint8_t frames_get_item = 4; // Legacy DEF_OBJECTGETITEM maxFrame=3 → 4 frames

    // Get frame count for current state
    uint8_t get_state_frame_count() const
    {
        switch (state)
        {
        case entity_anim_state::stop:
            return frames_idle;
        case entity_anim_state::move:
            return frames_walk;
        case entity_anim_state::run:
            return frames_run;
        case entity_anim_state::attack:
            return frames_attack;
        case entity_anim_state::attack_move:
            return frames_attack_move;
        case entity_anim_state::damage:
            return frames_damage;
        case entity_anim_state::damage_move:
            return frames_damage_move;
        case entity_anim_state::dying:
            return frames_dying;
        case entity_anim_state::dead:
            return frames_dead;
        case entity_anim_state::magic:
        case entity_anim_state::magic_attack:
            return frames_magic;
        case entity_anim_state::get_item:
            return frames_get_item;
        default:
            return frames_idle;
        }
    }

    // Get frame duration for current state (in seconds, matching legacy frame times)
    float get_state_frame_duration() const
    {
        switch (state)
        {
        case entity_anim_state::stop:
            return 0.115f; // 115ms per frame
        case entity_anim_state::move:
        case entity_anim_state::damage_move:
            return 0.070f; // 70ms per frame (legacy DEF_OBJECTMOVE)
        case entity_anim_state::run:
            return 0.042f; // 42ms per frame (legacy DEF_OBJECTRUN)
        case entity_anim_state::attack:
        case entity_anim_state::attack_move:
            return 0.078f; // 78ms per frame (legacy DEF_OBJECTATTACK)
        case entity_anim_state::magic:
        case entity_anim_state::magic_attack:
            return 0.088f; // 88ms per frame (legacy DEF_OBJECTMAGIC)
        case entity_anim_state::damage:
            return 0.070f; // 70ms per frame (legacy DEF_OBJECTDAMAGE)
        case entity_anim_state::dying:
            return 0.080f; // 80ms per frame (legacy DEF_OBJECTDYING)
        case entity_anim_state::get_item:
            return 0.150f; // 150ms per frame (legacy DEF_OBJECTGETITEM)
        default:
            return 0.060f; // Default to idle timing
        }
    }

    // Should this state loop?
    bool should_loop() const
    {
        switch (state)
        {
        case entity_anim_state::stop:
        case entity_anim_state::move:
        case entity_anim_state::run:
        case entity_anim_state::dead:
            return true;
        case entity_anim_state::attack:
        case entity_anim_state::attack_move:
        case entity_anim_state::damage:
        case entity_anim_state::damage_move:
        case entity_anim_state::dying:
        case entity_anim_state::magic:
        case entity_anim_state::magic_attack:
        case entity_anim_state::get_item:
            return false;
        default:
            return true;
        }
    }

    // Set animation state with proper initialization
    void set_state(entity_anim_state new_state)
    {
        if (state == new_state && !finished)
            return;
        state = new_state;
        current_frame = 0;
        frame_timer = 0.0f;
        frame_count = get_state_frame_count();
        frame_duration = get_state_frame_duration();
        looping = should_loop();
        finished = false;
        attack_triggered = false;
    }
};

// Stats component - character attributes
struct stats_component
{
    // Base stats
    uint16_t strength = 10;
    uint16_t vitality = 10;
    uint16_t dexterity = 10;
    uint16_t intelligence = 10;
    uint16_t magic = 10;
    uint16_t charisma = 10;

    // Current/max values
    int32_t hp = 100;
    int32_t max_hp = 100;
    int32_t mp = 50;
    int32_t max_mp = 50;
    int32_t sp = 100;
    int32_t max_sp = 100;

    // Level/XP
    uint16_t level = 1;
    int64_t experience = 0;
    int64_t experience_to_next = 100;

    // Combat stats
    int32_t attack_power = 10;
    int32_t magic_power = 10;
    int32_t defense = 5;
    int32_t magic_resist = 5;
    int32_t hit_ratio = 50;
    int32_t dodge_ratio = 10;
    int32_t critical_ratio = 5;

    // Status
    uint8_t hunger = 100;

    // Contribution score (server-provided)
    int32_t contribution = 0;
};

// Combat component - combat state
struct combat_component
{
    enum class combat_mode : uint8_t
    {
        peaceful = 0,
        attack_player = 1,
        attack_monster = 2,
        full_attack = 3,
    };

    combat_mode mode = combat_mode::attack_monster;
    bool combat_stance = false; // Visual combat stance (combat idle/walk animations)
    uint32_t target_id = 0;
    uint8_t attack_type = 0;
    bool is_attacking = false;
    bool is_casting = false;
    float attack_cooldown = 0.0f;
    float spell_cooldown = 0.0f;
    uint16_t current_spell = 0;

    // Status effects
    bool poisoned = false;
    bool paralyzed = false;
    bool frozen = false;
    bool invisible = false;
    bool invulnerable = false;
    float status_duration = 0.0f;

    // PK status
    int32_t pk_count = 0;
    int32_t enemy_kill_count = 0;
};

// Name component - display name and guild
struct name_component
{
    std::string name;
    std::string guild_name;
    std::string guild_tag;
    std::string guild_rank;
    uint8_t guild_rank_id = 255; // 255 = not in guild, 0 = master
    uint32_t guild_id = 0;

    std::string faction;                         // "neutral", "aresden", "elvine"
    enum hostility hostile = hostility::neutral; // server-authoritative
    enum pk_status pk = pk_status::innocent;

    // Chat bubble
    std::string chat_message;
    float chat_timer = 0.0f;
    float chat_elapsed = 0.0f;
    text_style chat_style = {sf::Color::White, sf::Color::Black, 1.0f, 12, text_effect::none};

    // Title/status
    std::string title;
    bool is_gm = false;
};

// Movement component - movement properties
struct movement_component
{
    float speed = 4.0f; // Tiles per second
    float run_speed = 8.0f;
    bool running = false;
    bool moving = false;
    bool can_move = true;

    // Movement target
    int32_t target_x = 0;
    int32_t target_y = 0;

    // Pathfinding
    std::vector<std::pair<int32_t, int32_t>> path;
    size_t path_index = 0;
};

// NPC-specific component
struct npc_component
{
    uint16_t npc_type = 0;
    uint16_t npc_id = 0;

    bool is_shop = false;
    bool is_quest_giver = false;
    bool is_banker = false;
    bool is_warehouse = false;
    bool is_skill_trainer = false;

    // Dialog
    std::string greeting;
    std::vector<std::string> dialog_options;
};

// Monster-specific component
struct monster_component
{
    uint16_t monster_type = 0;
    uint8_t monster_level = 1;
    uint32_t owner_id = 0; // For summons

    bool is_boss = false;
    bool is_summon = false;
    bool is_aggressive = true;

    // Special abilities (1-7): Clairvoyant, Dest. Magic Prot., Anti-Physical,
    // Anti-Magic, Poisonous, Critical Poisonous, Explosive
    // Multiple possible but rare; more attributes = more exp
    std::vector<std::string> attributes;

    std::string category;                      // "monster", "boss", "guard", "merchant", etc.
    enum hostility hostile = hostility::enemy; // default enemy for monsters

    // Status effects (can toggle on/off at runtime)
    bool berserked = false;
    bool frozen = false;

    // Loot
    uint32_t exp_reward = 0;
    uint32_t gold_reward = 0;
};

// Item drop component
struct item_component
{
    uint32_t item_id = 0;
    uint16_t item_type = 0;
    uint16_t item_sprite = 0;
    uint32_t amount = 1;
    uint32_t owner_id = 0;     // Who can pick it up (0 = anyone)
    float pickup_timer = 0.0f; // Time until anyone can pick up
};

// Effect component - visual effects
struct effect_component
{
    uint16_t effect_type = 0;
    const sprite* effect_sprite = nullptr;
    uint8_t effect_frame = 0;
    float effect_timer = 0.0f;
    float effect_duration = 1.0f;
    bool attached_to_entity = false;
    int32_t offset_x = 0;
    int32_t offset_y = 0;
};

} // namespace hb
