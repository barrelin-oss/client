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
    skill_category category = skill_category::combat;

    // Mastery level (0-100), set directly from server
    uint8_t mastery = 0;

    // Sub-level progress toward next level (0.0-1.0), from uses_this_level / uses_to_next_level
    float sub_progress = 0.0f;

    // Whether actively usable (false = passive)
    bool is_useable = false;

    // Use method: 0 = direct, 1 = item-based, 2 = dialog-based
    uint8_t use_method = 0;

    uint8_t level() const { return mastery; }
};

class skills_system {
public:
    skills_system() = default;
    ~skills_system() = default;

    // Initialization - loads from assets/data/config/skills.yaml
    void initialize();
    void clear();

    // Skill access
    const skill* get_skill(uint16_t skill_id) const;
    skill* get_skill_mut(uint16_t skill_id);
    std::vector<const skill*> get_skills_by_category(skill_category category) const;
    std::vector<const skill*> get_all_skills() const;

    // Set skill level from server data
    void set_mastery(uint16_t skill_id, uint8_t mastery);
    void set_sub_progress(uint16_t skill_id, float progress);
    uint8_t get_skill_level(uint16_t skill_id) const;

    // Weapon mastery check
    [[nodiscard]] bool is_skill_mastered(uint16_t skill_id) const noexcept;

    // Active skill usage
    bool can_use_skill(uint16_t skill_id) const;
    void update_cooldowns(float delta_time);
    bool is_skill_ready(uint16_t skill_id) const;
    void trigger_cooldown(uint16_t skill_id);

    // Callbacks
    using skill_up_callback = std::function<void(uint16_t skill_id, uint8_t new_level)>;
    void set_skill_up_callback(skill_up_callback cb) { on_skill_up_ = std::move(cb); }

private:
    void load_from_yaml();

    std::unordered_map<uint16_t, skill> skills_;
    std::unordered_map<uint16_t, float> cooldowns_;
    skill_up_callback on_skill_up_;
};

// Skill ID constants matching legacy skillcfg.txt / server IDs
namespace skill_id {
    // Gathering
    inline constexpr uint16_t mining = 0;
    inline constexpr uint16_t fishing = 1;
    inline constexpr uint16_t farming = 2;

    // Magic
    inline constexpr uint16_t magic_resistance = 3;
    inline constexpr uint16_t magic = 4;

    // Combat
    inline constexpr uint16_t hand_attack = 5;
    inline constexpr uint16_t archery = 6;
    inline constexpr uint16_t short_sword = 7;
    inline constexpr uint16_t long_sword = 8;
    inline constexpr uint16_t fencing = 9;
    inline constexpr uint16_t axe = 10;
    inline constexpr uint16_t shield = 11;

    // Crafting
    inline constexpr uint16_t alchemy = 12;
    inline constexpr uint16_t manufacturing = 13;
    inline constexpr uint16_t hammer = 14;
    inline constexpr uint16_t crafting = 16;

    // Misc
    inline constexpr uint16_t pretend_corpse = 19;
    inline constexpr uint16_t staff = 21;
    inline constexpr uint16_t poison_resistance = 23;
}

} // namespace hb
