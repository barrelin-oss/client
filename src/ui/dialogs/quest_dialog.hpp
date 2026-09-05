#pragma once

// Quest dialog: the list of quests an officer offers (opened from the NPC dialog action
// open_quests, or quest_list_response) and the player's journal (J key, or the Quest
// button of the character dialog). One list on the left, the selected quest's details
// on the right, and the buttons the quest's status allows.

#include "ui/ui_system.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hb
{

struct quest_data;

struct quest_view_objective
{
    std::string description;
    int32_t current = 0;
    int32_t required = 0;
    bool complete = false;
};

struct quest_view
{
    uint32_t quest_id = 0;
    std::string name;
    std::string description;
    std::string status; // available, active, complete, turned_in, failed, abandoned
    int32_t min_level = 0;
    int32_t max_level = 0;
    bool repeatable = false;
    uint32_t giver_npc_id = 0;
    std::vector<quest_view_objective> objectives;
    uint32_t reward_experience = 0;
    uint32_t reward_gold = 0;
    std::vector<std::string> reward_items;

    static quest_view from_data(const quest_data& d);
};

class quest_dialog : public dialog
{
public:
    quest_dialog();
    ~quest_dialog() override = default;

    void update(float delta_time, const input& inp) override;
    void render(renderer& rend) override;
    bool handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn) override;
    bool handle_mouse_move(int32_t x, int32_t y) override;

    // Quests offered/tracked by one officer (quest_list_response). Opens the dialog.
    void show_offers(uint32_t npc_entity_id, std::string_view npc_name, std::vector<quest_view> quests);
    // The player's active quests (quest_journal_response). Opens the dialog.
    void show_journal(std::vector<quest_view> quests);
    // quest_update push: replaces or adds one quest, keeps the selection
    void apply_update(const quest_view& quest);
    void remove_quest(uint32_t quest_id);

    bool from_npc() const { return npc_entity_id_ != 0; }
    uint32_t npc_entity_id() const { return npc_entity_id_; }

    using npc_quest_callback = std::function<void(uint32_t npc_entity_id, uint32_t quest_id)>;
    using quest_callback = std::function<void(uint32_t quest_id)>;
    void set_on_accept(npc_quest_callback cb) { on_accept_ = std::move(cb); }
    void set_on_complete(npc_quest_callback cb) { on_complete_ = std::move(cb); }
    void set_on_abandon(quest_callback cb) { on_abandon_ = std::move(cb); }

private:
    struct button
    {
        std::string label;
        ui_rect rect;
        sf::Color color;
        int32_t action = 0; // 1 accept, 2 complete, 3 abandon, 4 close
    };
    std::vector<button> buttons_for_selected() const;
    const quest_view* selected() const;
    void draw_wrapped(renderer& rend, std::string_view text, int32_t x, int32_t& y, int32_t max_width,
                      int32_t max_y, sf::Color color) const;

    uint32_t npc_entity_id_ = 0;
    std::string npc_name_;
    std::vector<quest_view> quests_;
    std::optional<size_t> selected_index_;
    std::optional<size_t> hovered_index_;

    npc_quest_callback on_accept_;
    npc_quest_callback on_complete_;
    quest_callback on_abandon_;

    static constexpr int32_t list_width = 170;
    static constexpr int32_t row_height = 20;
    static constexpr int32_t list_top = 32;
};

} // namespace hb
