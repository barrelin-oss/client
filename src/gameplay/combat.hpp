#pragma once

#include "audio/sound_types.hpp"
#include "core/game_enums.hpp"
#include "entity/entity.hpp"
#include <cstdint>
#include <functional>

namespace hb
{

class entity_manager;
class inventory_system;
class magic_system;
class skills_system;
class sound_manager;

// Weapon skill types (from original game)
enum class weapon_skill : uint8_t
{
    fist = 5,     // Unarmed combat
    archery = 6,  // Bow and arrow
    two_hand = 7, // Two-handed weapons
    sword = 8,    // Swords
    spear = 9,    // Spears
    axe = 10,     // Axes
    hammer = 11,  // Hammers
    staff = 21,   // Magical staves
    shield = 14,  // Shield blocking
};

// Weapon type 40+ are bows/crossbows (ranged weapons)
inline bool is_bow_weapon(uint16_t weapon_type)
{
    return weapon_type >= 40;
}

// Attack types
enum class attack_type : uint8_t
{
    normal = 1,   // Standard melee attack
    ranged = 2,   // Bow/arrow attack
    dash = 3,     // Dash/lunge attack (2-tile range, requires mastered weapon skill)
    super_1 = 20, // First super attack (100% mastery)
    super_2 = 21, // Second super attack
    super_3 = 22, // Third super attack
    super_4 = 23, // Fourth super attack
    super_5 = 24, // Fifth super attack
    super_6 = 25, // Sixth super attack
    super_7 = 26, // Seventh super attack
    super_8 = 27, // Eighth super attack
};

// Damage result
struct damage_result
{
    int32_t damage = 0;
    bool is_critical = false;
    bool is_blocked = false;
    bool is_dodged = false;
    bool target_killed = false;
    int32_t experience_gained = 0;
};

// Attack parameters
struct attack_params
{
    entity_id attacker = invalid_entity_id;
    entity_id target = invalid_entity_id;
    attack_type type = attack_type::normal;
    uint16_t weapon_type = 0;
    int32_t attack_power = 0;
    int32_t hit_ratio = 0;
    int32_t critical_ratio = 0;
    int32_t target_defense = 0;
    int32_t target_dodge = 0;
};

// Stat formulas - calculate max values from base stats
struct stat_formulas
{
    // max_hp = (vitality * 3) + (level * 2) + (strength / 2)
    static int32_t calculate_max_hp(uint16_t vitality, uint16_t level, uint16_t strength)
    {
        return (vitality * 3) + (level * 2) + (strength / 2);
    }

    // max_mp = (magic * 2) + (level * 2) + (intelligence / 2)
    static int32_t calculate_max_mp(uint16_t magic, uint16_t level, uint16_t intelligence)
    {
        return (magic * 2) + (level * 2) + (intelligence / 2);
    }

    // max_sp = (strength * 2) + (level * 2)
    static int32_t calculate_max_sp(uint16_t strength, uint16_t level) { return (strength * 2) + (level * 2); }

    // Attack power base calculation
    static int32_t calculate_attack_power(uint16_t strength, uint16_t dexterity, uint16_t level)
    {
        return (strength / 5) + (dexterity / 10) + level;
    }

    // Magic power base calculation
    static int32_t calculate_magic_power(uint16_t magic, uint16_t intelligence, uint16_t level)
    {
        return (magic / 3) + (intelligence / 5) + level;
    }

    // Defense calculation
    static int32_t calculate_defense(uint16_t vitality, int32_t armor_defense)
    {
        return (vitality / 10) + armor_defense;
    }

    // Hit ratio calculation
    static int32_t calculate_hit_ratio(uint16_t dexterity, uint16_t level)
    {
        return 50 + (dexterity / 2) + (level / 2);
    }

    // Dodge ratio calculation
    static int32_t calculate_dodge_ratio(uint16_t dexterity, uint16_t level) { return (dexterity / 4) + (level / 4); }

    // Critical ratio calculation
    static int32_t calculate_critical_ratio(uint16_t dexterity, uint16_t level)
    {
        return 5 + (dexterity / 20) + (level / 10);
    }
};

// Combat callbacks
struct combat_callbacks
{
    std::function<void(entity_id attacker, entity_id target, const damage_result& result)> on_damage;
    std::function<void(entity_id attacker, entity_id target, attack_type type)> on_attack_start;
    std::function<void(entity_id entity)> on_death;
    std::function<void(entity_id entity, int32_t experience)> on_experience_gained;
    std::function<void(entity_id entity)> on_level_up;
};

class combat_system
{
public:
    combat_system() = default;
    ~combat_system() = default;

    // Initialization
    void initialize(entity_manager* entities,
                    magic_system* magic,
                    skills_system* skills,
                    sound_manager* sounds = nullptr,
                    inventory_system* inventory = nullptr);
    void update(float delta_time);

    // Combat actions
    damage_result calculate_damage(const attack_params& params);
    bool can_attack(entity_id attacker, entity_id target) const;
    void start_attack(entity_id attacker, entity_id target, attack_type type = attack_type::normal);
    void process_attack(entity_id attacker, entity_id target, int32_t damage);

    // Attack type determination
    attack_type get_attack_type(uint16_t weapon_type, uint8_t skill_mastery, bool use_super) const;
    weapon_skill get_weapon_skill(uint16_t weapon_type) const;

    // Range checking
    bool is_in_melee_range(entity_id attacker, entity_id target) const;
    bool is_in_dash_range(entity_id attacker, entity_id target) const;
    bool is_in_ranged_range(entity_id attacker, entity_id target) const;
    int32_t get_attack_range(attack_type type, uint16_t weapon_type) const;

    // Dash attack check - requires mastered weapon skill and non-bow weapon
    bool can_dash_attack(entity_id attacker) const;

    // Super attack requirements
    bool can_use_super_attack(entity_id attacker, attack_type super_type) const;

    // Stat updates
    void apply_damage(entity_id target, int32_t damage);
    void heal_entity(entity_id target, int32_t amount);
    void restore_mp(entity_id target, int32_t amount);
    void restore_sp(entity_id target, int32_t amount);

    // Death handling
    void kill_entity(entity_id entity);
    bool is_dead(entity_id entity) const;

    // Experience and leveling
    void award_experience(entity_id entity, int32_t experience);
    bool check_level_up(entity_id entity);

    // Callbacks
    void set_callbacks(const combat_callbacks& callbacks) { callbacks_ = callbacks; }

    // PK system
    void add_pk_count(entity_id killer);
    void reduce_pk_count(entity_id entity, int32_t amount = 1);

private:
    bool roll_hit(int32_t hit_ratio, int32_t dodge_ratio) const;
    bool roll_critical(int32_t critical_ratio) const;
    int32_t calculate_base_damage(int32_t attack_power, int32_t defense) const;
    int32_t apply_skill_bonus(int32_t damage, weapon_skill skill, uint8_t mastery) const;
    int32_t apply_critical_bonus(int32_t damage) const;

    // Sound helper - get player type from entity for gendered sounds
    int get_player_type(entity_id id) const;

    // Play combat sounds
    void play_attack_sound(entity_id attacker, uint16_t weapon_type);
    void play_hurt_sound(entity_id target);
    void play_death_sound(entity_id target);
    void play_critical_sound(entity_id attacker);
    void play_level_up_sound(entity_id target);

    entity_manager* entities_ = nullptr;
    magic_system* magic_ = nullptr;
    skills_system* skills_ = nullptr;
    sound_manager* sounds_ = nullptr;
    inventory_system* inventory_ = nullptr;
    combat_callbacks callbacks_;
};

} // namespace hb
