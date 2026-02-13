#include "gameplay/skills.hpp"
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

namespace {

skill_category parse_category(const std::string& str)
{
    if (str == "combat") return skill_category::combat;
    if (str == "magic") return skill_category::magic;
    if (str == "crafting") return skill_category::crafting;
    if (str == "gathering") return skill_category::gathering;
    if (str == "misc") return skill_category::misc;
    spdlog::warn("Unknown skill category '{}', defaulting to misc", str);
    return skill_category::misc;
}

} // anonymous namespace

void skills_system::initialize()
{
    load_from_yaml();
    spdlog::info("Skills system initialized with {} skills", skills_.size());
}

void skills_system::clear()
{
    for (auto& [id, sk] : skills_)
    {
        sk.mastery = 0;
        sk.sub_progress = 0.0f;
    }
    cooldowns_.clear();
}

const skill* skills_system::get_skill(uint16_t skill_id) const
{
    auto it = skills_.find(skill_id);
    return it != skills_.end() ? &it->second : nullptr;
}

skill* skills_system::get_skill_mut(uint16_t skill_id)
{
    auto it = skills_.find(skill_id);
    return it != skills_.end() ? &it->second : nullptr;
}

std::vector<const skill*> skills_system::get_skills_by_category(skill_category category) const
{
    std::vector<const skill*> result;
    for (const auto& [id, sk] : skills_)
    {
        if (sk.category == category)
            result.push_back(&sk);
    }
    return result;
}

std::vector<const skill*> skills_system::get_all_skills() const
{
    std::vector<const skill*> result;
    for (const auto& [id, sk] : skills_)
        result.push_back(&sk);
    return result;
}

void skills_system::set_mastery(uint16_t skill_id, uint8_t mastery)
{
    auto it = skills_.find(skill_id);
    if (it != skills_.end())
    {
        uint8_t old_level = it->second.mastery;
        it->second.mastery = std::min(mastery, static_cast<uint8_t>(100));

        if (it->second.mastery > old_level && on_skill_up_)
            on_skill_up_(skill_id, it->second.mastery);
    }
}

void skills_system::set_sub_progress(uint16_t skill_id, float progress)
{
    auto it = skills_.find(skill_id);
    if (it != skills_.end())
        it->second.sub_progress = std::clamp(progress, 0.0f, 1.0f);
}

uint8_t skills_system::get_skill_level(uint16_t skill_id) const
{
    auto it = skills_.find(skill_id);
    return it != skills_.end() ? it->second.mastery : 0;
}

bool skills_system::is_skill_mastered(uint16_t skill_id) const noexcept
{
    auto it = skills_.find(skill_id);
    return it != skills_.end() && it->second.mastery >= 100;
}

bool skills_system::can_use_skill(uint16_t skill_id) const
{
    auto* s = get_skill(skill_id);
    if (!s || !s->is_useable) return false;
    return is_skill_ready(skill_id);
}

void skills_system::update_cooldowns(float delta_time)
{
    for (auto it = cooldowns_.begin(); it != cooldowns_.end(); )
    {
        it->second -= delta_time;
        if (it->second <= 0)
            it = cooldowns_.erase(it);
        else
            ++it;
    }
}

bool skills_system::is_skill_ready(uint16_t skill_id) const
{
    return cooldowns_.find(skill_id) == cooldowns_.end();
}

void skills_system::trigger_cooldown(uint16_t skill_id)
{
    // Cooldown could be data-driven later; for now just a short default
    cooldowns_[skill_id] = 1.0f;
}

void skills_system::load_from_yaml()
{
    const std::string path = "assets/data/config/skills.yaml";
    try
    {
        YAML::Node root = YAML::LoadFile(path);
        if (!root["skills"] || !root["skills"].IsSequence())
        {
            spdlog::error("skills.yaml: missing or invalid 'skills' array");
            return;
        }

        for (const auto& node : root["skills"])
        {
            skill sk;
            sk.id = node["id"].as<uint16_t>();
            sk.name = node["name"].as<std::string>();
            sk.category = parse_category(node["category"].as<std::string>());
            sk.is_useable = node["is_useable"].as<bool>(false);
            sk.use_method = node["use_method"].as<uint8_t>(0);
            skills_[sk.id] = std::move(sk);
        }
    }
    catch (const YAML::Exception& e)
    {
        spdlog::error("Failed to load {}: {}", path, e.what());
    }
}

} // namespace hb
