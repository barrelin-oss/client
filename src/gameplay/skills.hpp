#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>

namespace hb {

// Skill categories
enum class skill_category : uint8_t {
    combat = 0,
    magic = 1,
    crafting = 2,
    gathering = 3,
    misc = 4,
};

// Skill data
struct skill {
    uint16_t id = 0;
    std::string name;
    std::string description;
    skill_category category = skill_category::combat;

    // Progress (0-100% displayed, internally 0-1000000)
    uint32_t experience = 0;
    static constexpr uint32_t max_experience = 1000000;

    // Requirements
    uint16_t level_req = 0;
    uint16_t stat_req = 0;         // Required stat value
    uint16_t prerequisite_skill = 0; // Must master this first

    // Whether actively usable
    bool is_passive = true;
    uint16_t mp_cost = 0;
    uint16_t sp_cost = 0;
    float cooldown = 0.0f;

    // Sub-level progress toward next level (0.0-1.0), sent by server
    float sub_progress = 0.0f;

    // Get skill level (0-100)
    uint8_t level() const {
        return static_cast<uint8_t>(experience / 10000);
    }

    // Get percentage progress (overall 0-100%)
    float progress() const {
        return static_cast<float>(experience) / static_cast<float>(max_experience) * 100.0f;
    }
};

class skills_system {
public:
    skills_system() = default;
    ~skills_system() = default;

    // Initialization
    void initialize();
    void clear();

    // Skill access
    const skill* get_skill(uint16_t skill_id) const;
    skill* get_skill_mut(uint16_t skill_id);
    std::vector<const skill*> get_skills_by_category(skill_category category) const;
    std::vector<const skill*> get_all_skills() const;

    // Experience
    void add_skill_experience(uint16_t skill_id, uint32_t exp);
    void set_skill_experience(uint16_t skill_id, uint32_t exp);
    void set_mastery(uint16_t skill_id, uint8_t mastery);  // Set skill level (0-100)
    void set_sub_progress(uint16_t skill_id, float progress);  // Set sub-level progress (0.0-1.0)
    uint8_t get_skill_level(uint16_t skill_id) const;

    // Check skill-based bonuses
    float get_damage_bonus() const;       // From combat skills
    float get_defense_bonus() const;      // From defensive skills
    float get_crafting_success_rate(uint16_t recipe_id) const;
    float get_gathering_bonus() const;

    // Weapon mastery check - efficient for per-frame rendering
    // Returns true if the skill is at 100% mastery (level >= 100)
    [[nodiscard]] bool is_skill_mastered(uint16_t skill_id) const noexcept;

    // Active skill usage
    bool can_use_skill(uint16_t skill_id, int32_t mp, int32_t sp) const;
    void update_cooldowns(float delta_time);
    bool is_skill_ready(uint16_t skill_id) const;
    void trigger_cooldown(uint16_t skill_id);

    // Callbacks
    using skill_up_callback = std::function<void(uint16_t skill_id, uint8_t new_level)>;
    void set_skill_up_callback(skill_up_callback cb) { on_skill_up_ = std::move(cb); }

    // Load skill definitions
    void load_skill_definitions();

private:
    void load_default_skills();

    std::unordered_map<uint16_t, skill> skills_;
    std::unordered_map<uint16_t, float> cooldowns_;
    skill_up_callback on_skill_up_;
};

// Skill ID constants
namespace skill_id {
    // Combat skills
    inline constexpr uint16_t attack = 1;
    inline constexpr uint16_t defense = 2;
    inline constexpr uint16_t critical_hit = 3;
    inline constexpr uint16_t shield_block = 4;
    inline constexpr uint16_t dual_wield = 5;
    inline constexpr uint16_t archery = 6;
    inline constexpr uint16_t short_sword = 7;
    inline constexpr uint16_t long_sword = 8;
    inline constexpr uint16_t axe = 9;
    inline constexpr uint16_t hammer = 10;
    inline constexpr uint16_t staff = 11;

    // Magic skills
    inline constexpr uint16_t magic_resistance = 20;
    inline constexpr uint16_t meditation = 21;
    inline constexpr uint16_t mana_control = 22;
    inline constexpr uint16_t fire_magic = 23;
    inline constexpr uint16_t ice_magic = 24;
    inline constexpr uint16_t lightning_magic = 25;
    inline constexpr uint16_t dark_magic = 26;
    inline constexpr uint16_t holy_magic = 27;

    // Crafting skills
    inline constexpr uint16_t alchemy = 40;
    inline constexpr uint16_t blacksmithing = 41;
    inline constexpr uint16_t tailoring = 42;
    inline constexpr uint16_t manufacturing = 43;
    inline constexpr uint16_t enchanting = 44;

    // Gathering skills
    inline constexpr uint16_t mining = 60;
    inline constexpr uint16_t fishing = 61;
    inline constexpr uint16_t farming = 62;

    // Misc skills
    inline constexpr uint16_t pretend_corpse = 80;
    inline constexpr uint16_t poison_resistance = 81;
}

} // namespace hb
