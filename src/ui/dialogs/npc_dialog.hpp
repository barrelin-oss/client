#pragma once

#include "ui/ui_system.hpp"
#include <functional>
#include <vector>
#include <string>

namespace hb
{

// NPC dialog option
struct npc_option
{
    std::string text;
    int32_t id = 0;
    bool enabled = true;
};

// NPC dialog - for conversations with NPCs
class npc_dialog : public dialog
{
public:
    npc_dialog();
    ~npc_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Set NPC info
    void set_npc_name(std::string_view name) { npc_name_ = name; }
    void set_portrait_sprite(uint16_t sprite_id) { portrait_sprite_ = sprite_id; }

    // Set dialog content
    void set_message(std::string_view message) { message_ = message; }
    void set_options(const std::vector<npc_option>& options);
    void add_option(const npc_option& option);
    void clear_options();

    // Callbacks
    using option_callback = std::function<void(int32_t option_id)>;
    void set_on_option_select(option_callback callback) { on_option_select_ = std::move(callback); }

private:
    std::string npc_name_;
    uint16_t portrait_sprite_ = 0;
    std::string message_;
    std::vector<npc_option> options_;

    std::optional<size_t> hovered_option_;

    option_callback on_option_select_;

    static constexpr int32_t option_height = 24;
    int32_t options_start_y_ = 0;
};

// Quest dialog - for quest-related NPC interactions
class quest_dialog : public dialog
{
public:
    quest_dialog();
    ~quest_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;

    // Quest info
    void set_quest_name(std::string_view name) { quest_name_ = name; }
    void set_quest_description(std::string_view desc) { description_ = desc; }
    void set_objectives(const std::vector<std::string>& objectives);

    struct quest_reward
    {
        std::string description;
        uint32_t gold = 0;
        uint32_t exp = 0;
    };
    void set_rewards(const quest_reward& rewards) { rewards_ = rewards; }

    // Callbacks
    using action_callback = std::function<void()>;
    void set_on_accept(action_callback callback) { on_accept_ = std::move(callback); }
    void set_on_decline(action_callback callback) { on_decline_ = std::move(callback); }
    void set_on_complete(action_callback callback) { on_complete_ = std::move(callback); }

    // Quest state
    enum class quest_state
    {
        offer,
        in_progress,
        completable
    };
    void set_state(quest_state state) { state_ = state; }

private:
    std::string quest_name_;
    std::string description_;
    std::vector<std::string> objectives_;
    quest_reward rewards_;
    quest_state state_ = quest_state::offer;

    action_callback on_accept_;
    action_callback on_decline_;
    action_callback on_complete_;
};

} // namespace hb
