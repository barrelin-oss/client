#pragma once

#include "core/game_enums.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>

namespace hb {

class entity_manager;
class combat_system;
class effect_system;

// Element types
enum class magic_element : uint8_t {
    none = 0,
    fire = 1,
    ice = 2,
    lightning = 3,
    earth = 4,
    holy = 5,
    dark = 6,
};

// Spell categories
enum class spell_category : uint8_t {
    attack = 0,
    defense = 1,
    healing = 2,
    buff = 3,
    debuff = 4,
    summon = 5,
    utility = 6,
};

// Spell target type
enum class spell_target : uint8_t {
    self = 0,
    single = 1,      // Single target (enemy or ally)
    area = 2,        // Area of effect
    ground = 3,      // Target location
    party = 4,       // All party members
    all_enemies = 5, // All enemies in range
};

// Casting state
enum class casting_state : uint8_t {
    idle = 0,
    casting = 1,
    finished = 2,
    interrupted = 3,
};

// Spell data
struct spell {
    uint16_t id = 0;
    std::string name;
    std::string description;

    spell_category category = spell_category::attack;
    spell_target target_type = spell_target::single;
    magic_type type = magic_type::damage_spot;
    magic_element element = magic_element::none;

    // Circle (spell tier 1-6)
    uint8_t circle = 1;

    // Requirements
    uint16_t level_req = 1;
    uint16_t magic_req = 0;
    uint16_t int_req = 0;

    // Cost
    uint16_t mp_cost = 0;
    uint16_t sp_cost = 0;
    uint16_t hp_cost = 0;

    // Effect
    int32_t base_damage = 0;
    int32_t base_healing = 0;
    float duration = 0.0f;       // For buffs/debuffs
    float cooldown = 1.0f;       // Seconds
    int32_t range = 8;           // In tiles
    int32_t aoe_radius = 0;      // For area spells

    // Animation
    uint16_t cast_sprite = 0;
    uint16_t projectile_effect = 0;  // Projectile from caster to target (0 = none)
    uint16_t effect_sprite = 0;      // Impact effect at target
    float cast_time = 1.0f;          // Seconds

    // Learning
    bool learned = false;
    uint8_t mastery_level = 0;   // 0-20
    uint32_t experience = 0;
    int32_t total_casts = 0;     // Lifetime cast count from server

    // Calculate actual mana cost based on intelligence and mastery
    int32_t calculate_mp_cost(uint16_t intelligence) const {
        // Base cost reduced by intelligence and mastery
        int32_t cost = mp_cost;
        cost -= (intelligence / 10);
        cost -= (mastery_level / 2);
        return std::max(1, cost);
    }

    // Calculate actual damage based on magic power and mastery
    int32_t calculate_damage(int32_t magic_power) const {
        float mastery_bonus = 1.0f + (mastery_level * 0.05f);
        return static_cast<int32_t>((base_damage + magic_power / 2) * mastery_bonus);
    }

    // Calculate actual healing based on magic power and mastery
    int32_t calculate_healing(int32_t magic_power) const {
        float mastery_bonus = 1.0f + (mastery_level * 0.05f);
        return static_cast<int32_t>((base_healing + magic_power / 3) * mastery_bonus);
    }
};

// Spell effect structure (for active effects)
struct spell_effect {
    uint16_t spell_id;
    uint32_t caster_id;
    uint32_t target_id;
    float remaining_duration;
    int32_t power;              // Damage/healing per tick
};

// Casting info
struct casting_info {
    uint16_t spell_id = 0;
    uint32_t target_id = 0;
    int32_t target_x = 0;
    int32_t target_y = 0;
    float cast_progress = 0.0f;
    float cast_duration = 0.0f;
    casting_state state = casting_state::idle;
};

// Spell cast result
struct spell_cast_result {
    bool success = false;
    int32_t damage_dealt = 0;
    int32_t healing_done = 0;
    int32_t mp_cost = 0;
    std::vector<uint32_t> affected_targets;
    std::string error_message;
};

class magic_system {
public:
    magic_system() = default;
    ~magic_system() = default;

    // Initialization
    void initialize();
    void initialize_with_systems(entity_manager* entities, combat_system* combat);
    void clear();

    // Spell management
    void learn_spell(uint16_t spell_id);
    void forget_spell(uint16_t spell_id);
    bool has_spell(uint16_t spell_id) const;
    bool can_cast(uint16_t spell_id, int32_t mp, int32_t sp, int32_t hp) const;
    bool can_cast_at_target(uint16_t spell_id, uint32_t caster_id, uint32_t target_id) const;
    bool can_cast_at_location(uint16_t spell_id, uint32_t caster_id, int32_t x, int32_t y) const;

    // Spell casting
    bool start_casting(uint16_t spell_id, uint32_t caster_id, uint32_t target_id);
    bool start_casting_at_location(uint16_t spell_id, uint32_t caster_id, int32_t x, int32_t y);
    void cancel_casting(uint32_t caster_id);
    void interrupt_casting(uint32_t caster_id);
    void update_casting(float delta_time);
    casting_state get_casting_state(uint32_t caster_id) const;
    float get_casting_progress(uint32_t caster_id) const;

    // Instant spell execution (for server-confirmed spells)
    spell_cast_result execute_spell(uint16_t spell_id, uint32_t caster_id, uint32_t target_id, int32_t magic_power);
    spell_cast_result execute_spell_at_location(uint16_t spell_id, uint32_t caster_id, int32_t x, int32_t y, int32_t magic_power);

    // Spell access
    const spell* get_spell(uint16_t spell_id) const;
    spell* get_spell_mut(uint16_t spell_id);
    const std::vector<const spell*> get_learned_spells() const;
    std::vector<const spell*> get_all_spells() const;
    const std::vector<const spell*> get_spells_by_category(spell_category category) const;
    std::vector<const spell*> get_spells_by_circle(uint8_t circle) const;

    // Mastery
    void add_spell_experience(uint16_t spell_id, uint32_t exp);
    void set_spell_mastery(uint16_t spell_id, uint8_t level);
    void set_spell_total_casts(uint16_t spell_id, int32_t total_casts);
    uint8_t get_mastery_level(uint16_t spell_id) const;

    // Active effects
    void add_effect(const spell_effect& effect);
    void remove_effect(uint16_t spell_id, uint32_t target_id);
    void update_effects(float delta_time);
    std::vector<spell_effect> get_effects_on_target(uint32_t target_id) const;
    bool has_effect(uint32_t target_id, uint16_t spell_id) const;

    // Set the effect system for visual effect delegation
    void set_effect_system(effect_system* effects) { effects_ = effects; }

    // Visual effects (for rendering spell animations)
    void create_effect(uint16_t effect_id, int32_t x, int32_t y);
    void add_active_effect(uint16_t effect_id, uint32_t duration);
    void remove_active_effect(uint16_t effect_id);

    // Cooldowns
    bool is_on_cooldown(uint16_t spell_id) const;
    float get_cooldown_remaining(uint16_t spell_id) const;
    void trigger_cooldown(uint16_t spell_id);
    void update_cooldowns(float delta_time);

    // Pending spell (for quick cast from spellbook)
    void set_pending_spell(uint16_t spell_id) { pending_spell_ = spell_id; }
    uint16_t pending_spell() const { return pending_spell_; }
    void clear_pending_spell() { pending_spell_ = 0; }
    bool has_pending_spell() const { return pending_spell_ != 0; }

    // Callbacks
    using effect_expired_callback = std::function<void(const spell_effect&)>;
    using spell_cast_callback = std::function<void(uint16_t spell_id, uint32_t caster_id, const spell_cast_result&)>;
    using casting_started_callback = std::function<void(uint16_t spell_id, uint32_t caster_id)>;
    using casting_interrupted_callback = std::function<void(uint32_t caster_id)>;

    void set_effect_expired_callback(effect_expired_callback cb) { on_effect_expired_ = std::move(cb); }
    void set_spell_cast_callback(spell_cast_callback cb) { on_spell_cast_ = std::move(cb); }
    void set_casting_started_callback(casting_started_callback cb) { on_casting_started_ = std::move(cb); }
    void set_casting_interrupted_callback(casting_interrupted_callback cb) { on_casting_interrupted_ = std::move(cb); }

    // Load spell definitions
    void load_spell_definitions();

private:
    void load_default_spells();
    void finish_casting(uint32_t caster_id);
    bool check_range(uint32_t caster_id, uint32_t target_id, int32_t range) const;
    bool check_range_location(uint32_t caster_id, int32_t x, int32_t y, int32_t range) const;

    std::unordered_map<uint16_t, spell> spells_;
    std::vector<spell_effect> active_effects_;
    std::unordered_map<uint32_t, casting_info> casting_entities_;
    std::unordered_map<uint16_t, float> cooldowns_;
    uint16_t pending_spell_ = 0;

    entity_manager* entities_ = nullptr;
    combat_system* combat_ = nullptr;
    effect_system* effects_ = nullptr;

    effect_expired_callback on_effect_expired_;
    spell_cast_callback on_spell_cast_;
    casting_started_callback on_casting_started_;
    casting_interrupted_callback on_casting_interrupted_;
};

// Spell ID constants matching server magic.yaml
namespace spell_id {
    // Level 1 (IDs 0-9)
    inline constexpr uint16_t magic_missile = 0;
    inline constexpr uint16_t heal = 1;
    inline constexpr uint16_t create_food = 2;

    // Level 2 (IDs 10-19)
    inline constexpr uint16_t energy_bolt = 10;
    inline constexpr uint16_t staminar_drain = 11;
    inline constexpr uint16_t recall = 12;
    inline constexpr uint16_t defense_shield = 13;
    inline constexpr uint16_t celebrating_light = 14;

    // Level 3 (IDs 20-29)
    inline constexpr uint16_t fire_ball = 20;
    inline constexpr uint16_t great_heal = 21;
    inline constexpr uint16_t staminar_recovery = 23;
    inline constexpr uint16_t protection_from_arrow = 24;
    inline constexpr uint16_t hold_person = 25;
    inline constexpr uint16_t possession = 26;
    inline constexpr uint16_t poison = 27;
    inline constexpr uint16_t great_staminar_recovery = 28;

    // Level 4 (IDs 30-39)
    inline constexpr uint16_t fire_strike = 30;
    inline constexpr uint16_t summon_creature = 31;
    inline constexpr uint16_t invisibility = 32;
    inline constexpr uint16_t protection_from_magic = 33;
    inline constexpr uint16_t detect_invisibility = 34;
    inline constexpr uint16_t paralyze = 35;
    inline constexpr uint16_t cure = 36;
    inline constexpr uint16_t lightning_arrow = 37;
    inline constexpr uint16_t tremor = 38;

    // Level 5 (IDs 40-49)
    inline constexpr uint16_t fire_wall = 40;
    inline constexpr uint16_t fire_field = 41;
    inline constexpr uint16_t confuse_language = 42;
    inline constexpr uint16_t lightning = 43;
    inline constexpr uint16_t great_defense_shield = 44;
    inline constexpr uint16_t chill_wind = 45;
    inline constexpr uint16_t poison_cloud = 46;
    inline constexpr uint16_t triple_energy_bolt = 47;

    // Level 6 (IDs 50-59)
    inline constexpr uint16_t berserk = 50;
    inline constexpr uint16_t lightning_bolt = 51;
    inline constexpr uint16_t mass_poison = 53;
    inline constexpr uint16_t spike_field = 54;
    inline constexpr uint16_t ice_storm = 55;
    inline constexpr uint16_t mass_lightning_arrow = 56;
    inline constexpr uint16_t ice_strike = 57;

    // Level 7 (IDs 60-69)
    inline constexpr uint16_t energy_strike = 60;
    inline constexpr uint16_t mass_fire_strike = 61;
    inline constexpr uint16_t confusion = 62;
    inline constexpr uint16_t mass_chill_wind = 63;
    inline constexpr uint16_t earthworm_strike = 64;
    inline constexpr uint16_t absolute_magic_protection = 65;
    inline constexpr uint16_t armor_break = 66;

    // Level 8 (IDs 70-79)
    inline constexpr uint16_t bloody_shock_wave = 70;
    inline constexpr uint16_t mass_confusion = 71;
    inline constexpr uint16_t mass_ice_strike = 72;
    inline constexpr uint16_t cloud_kill = 73;
    inline constexpr uint16_t lightning_strike = 74;
    inline constexpr uint16_t cancellation = 76;
    inline constexpr uint16_t illusion_movement = 77;

    // Level 9 (IDs 80-89)
    inline constexpr uint16_t illusion = 80;
    inline constexpr uint16_t meteor_strike = 81;
    inline constexpr uint16_t mass_magic_missile = 82;
    inline constexpr uint16_t inhibition_casting = 83;

    // Level 10 (IDs 90-99)
    inline constexpr uint16_t mass_illusion = 90;
    inline constexpr uint16_t blizzard = 91;
    inline constexpr uint16_t resurrection = 94;
    inline constexpr uint16_t mass_illusion_movement = 95;
    inline constexpr uint16_t earth_shock_wave = 96;
}

} // namespace hb
