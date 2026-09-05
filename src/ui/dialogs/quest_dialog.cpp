#include "ui/dialogs/quest_dialog.hpp"
#include "network/messages/quest.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "core/constants.hpp"
#include <algorithm>
#include <format>

namespace hb
{

namespace
{

sf::Color status_color(std::string_view status)
{
    if (status == "available")
        return sf::Color(255, 230, 120);
    if (status == "active")
        return sf::Color::White;
    if (status == "complete")
        return sf::Color(130, 255, 130);
    return sf::Color(150, 150, 150); // turned_in, failed, abandoned
}

std::string status_label(std::string_view status)
{
    if (status == "available")
        return "Available";
    if (status == "active")
        return "In progress";
    if (status == "complete")
        return "Ready to turn in";
    if (status == "turned_in")
        return "Turned in";
    return std::string(status);
}

} // namespace

quest_view quest_view::from_data(const quest_data& d)
{
    quest_view v;
    v.quest_id = d.quest_id;
    v.name = d.name;
    v.description = d.description;
    v.status = d.status;
    v.min_level = d.min_level;
    v.max_level = d.max_level;
    v.repeatable = d.repeatable;
    v.giver_npc_id = d.giver_npc_id;
    for (const auto& o : d.objectives)
        v.objectives.push_back({o.description, o.current, o.required, o.complete});
    v.reward_experience = d.reward_experience;
    v.reward_gold = d.reward_gold;
    v.reward_items = d.reward_items;
    return v;
}

quest_dialog::quest_dialog() : dialog(dialog_type::quest)
{
    set_title("Quests");
    set_bounds({static_cast<int32_t>(screen_width) / 2 - 230, static_cast<int32_t>(screen_height) / 2 - 170, 460, 340});
    set_draggable(true);
    set_closeable(true);
    set_visible(false);
}

void quest_dialog::update(float delta_time, const input& inp)
{
    dialog::update(delta_time, inp);
}

const quest_view* quest_dialog::selected() const
{
    if (!selected_index_ || *selected_index_ >= quests_.size())
        return nullptr;
    return &quests_[*selected_index_];
}

void quest_dialog::show_offers(uint32_t npc_entity_id, std::string_view npc_name, std::vector<quest_view> quests)
{
    npc_entity_id_ = npc_entity_id;
    npc_name_ = npc_name;
    quests_ = std::move(quests);
    selected_index_ = quests_.empty() ? std::nullopt : std::optional<size_t>(0);
    hovered_index_.reset();
    if (!is_open())
        open();
}

void quest_dialog::show_journal(std::vector<quest_view> quests)
{
    const uint32_t keep = selected() ? selected()->quest_id : 0;
    npc_entity_id_ = 0;
    npc_name_.clear();
    quests_ = std::move(quests);
    selected_index_.reset();
    for (size_t i = 0; i < quests_.size(); ++i)
    {
        if (quests_[i].quest_id == keep)
            selected_index_ = i;
    }
    if (!selected_index_ && !quests_.empty())
        selected_index_ = 0;
    hovered_index_.reset();
    if (!is_open())
        open();
}

void quest_dialog::apply_update(const quest_view& quest)
{
    for (auto& q : quests_)
    {
        if (q.quest_id == quest.quest_id)
        {
            q = quest;
            return;
        }
    }
    quests_.push_back(quest);
    if (!selected_index_)
        selected_index_ = 0;
}

void quest_dialog::remove_quest(uint32_t quest_id)
{
    const auto it = std::find_if(quests_.begin(), quests_.end(), [&](const quest_view& q) { return q.quest_id == quest_id; });
    if (it == quests_.end())
        return;
    quests_.erase(it);
    if (quests_.empty())
        selected_index_.reset();
    else if (selected_index_ && *selected_index_ >= quests_.size())
        selected_index_ = quests_.size() - 1;
}

std::vector<quest_dialog::button> quest_dialog::buttons_for_selected() const
{
    std::vector<button> out;
    const int32_t btn_y = bounds_.y + bounds_.height - 36;
    int32_t right = bounds_.x + bounds_.width - 10;
    auto add = [&](std::string label, sf::Color color, int32_t action)
    {
        constexpr int32_t w = 90;
        right -= w;
        out.push_back({std::move(label), ui_rect{right, btn_y, w, 26}, color, action});
        right -= 8;
    };
    add("Close", sf::Color(70, 70, 90), 4);
    if (const auto* q = selected())
    {
        if (from_npc() && q->status == "available")
            add("Accept", sf::Color(60, 100, 60), 1);
        if (from_npc() && q->status == "complete")
            add("Complete", sf::Color(100, 150, 60), 2);
        if (q->status == "active" || (!from_npc() && q->status == "complete"))
            add("Abandon", sf::Color(100, 60, 60), 3);
    }
    return out;
}

void quest_dialog::draw_wrapped(renderer& rend, std::string_view text, int32_t x, int32_t& y, int32_t max_width,
                                int32_t max_y, sf::Color color) const
{
    std::string remaining(text);
    const size_t chars_per_line = static_cast<size_t>(std::max(10, max_width / 6));
    while (!remaining.empty() && y < max_y)
    {
        std::string line;
        if (remaining.length() <= chars_per_line)
        {
            line = remaining;
            remaining.clear();
        }
        else
        {
            size_t break_pos = remaining.rfind(' ', chars_per_line);
            if (break_pos == std::string::npos || break_pos == 0)
                break_pos = chars_per_line;
            line = remaining.substr(0, break_pos);
            remaining = remaining.substr(std::min(remaining.size(), break_pos + 1));
        }
        rend.draw_text(line, x, y, color, 12);
        y += 14;
    }
}

void quest_dialog::render(renderer& rend)
{
    if (!visible_)
        return;

    dialog::render(rend);

    const int32_t x = bounds_.x + 8;
    int32_t y = bounds_.y + list_top;

    // Header: who offers these, or the journal
    if (from_npc())
        rend.draw_text(std::format("Tasks from {}", npc_name_.empty() ? "the officer" : npc_name_), x, y, sf::Color(180, 180, 220), 12);
    else
        rend.draw_text("Quest journal", x, y, sf::Color(180, 180, 220), 12);
    y += 18;

    // List
    const int32_t list_x = x;
    const int32_t list_y = y;
    const int32_t list_h = bounds_.height - (list_y - bounds_.y) - 46;
    rend.draw_rect(list_x, list_y, list_width, list_h, sf::Color(20, 20, 30, 200), true);
    if (quests_.empty())
    {
        rend.draw_text(from_npc() ? "Nothing to offer you now." : "No active quests.", list_x + 6, list_y + 6,
                       sf::Color(150, 150, 150), 12);
    }
    const size_t max_rows = static_cast<size_t>(std::max(1, list_h / row_height));
    for (size_t i = 0; i < quests_.size() && i < max_rows; ++i)
    {
        const int32_t ry = list_y + static_cast<int32_t>(i) * row_height;
        const bool sel = selected_index_ && *selected_index_ == i;
        const bool hov = hovered_index_ && *hovered_index_ == i;
        if (sel || hov)
            rend.draw_rect(list_x, ry, list_width, row_height, sel ? sf::Color(60, 60, 100) : sf::Color(45, 45, 70), true);
        std::string name = quests_[i].name;
        if (name.size() > 26)
            name = name.substr(0, 25) + "~";
        rend.draw_text(name, list_x + 6, ry + 3, status_color(quests_[i].status), 12);
    }

    // Details
    const int32_t dx = list_x + list_width + 10;
    const int32_t dw = bounds_.x + bounds_.width - dx - 8;
    int32_t dy = list_y;
    const int32_t detail_bottom = list_y + list_h;
    if (const auto* q = selected())
    {
        rend.draw_text(q->name, dx, dy, sf::Color(255, 230, 120), 14);
        dy += 18;
        std::string meta = status_label(q->status);
        if (q->min_level > 0 || q->max_level > 0)
            meta += std::format("  -  level {}-{}", q->min_level, q->max_level);
        if (q->repeatable)
            meta += "  -  repeatable";
        rend.draw_text(meta, dx, dy, status_color(q->status), 12);
        dy += 18;
        draw_wrapped(rend, q->description, dx, dy, dw, detail_bottom - 60, sf::Color(220, 220, 220));
        dy += 6;
        if (!q->objectives.empty())
        {
            rend.draw_text("Objectives:", dx, dy, sf::Color(150, 150, 180), 12);
            dy += 15;
            for (const auto& o : q->objectives)
            {
                if (dy > detail_bottom - 30)
                    break;
                std::string line = "- " + o.description;
                if (o.required > 0)
                    line += std::format(" ({}/{})", o.current, o.required);
                rend.draw_text(line, dx + 6, dy, o.complete ? sf::Color(130, 255, 130) : sf::Color::White, 12);
                dy += 14;
            }
            dy += 4;
        }
        std::string rewards;
        if (q->reward_experience > 0)
            rewards += std::format("{} exp", q->reward_experience);
        if (q->reward_gold > 0)
            rewards += std::format("{}{} gold", rewards.empty() ? "" : ", ", q->reward_gold);
        for (const auto& item : q->reward_items)
            rewards += (rewards.empty() ? "" : ", ") + item;
        if (!rewards.empty() && dy <= detail_bottom - 14)
            rend.draw_text("Rewards: " + rewards, dx, dy, sf::Color(255, 220, 100), 12);
    }

    // Buttons
    for (const auto& b : buttons_for_selected())
    {
        rend.draw_rect(b.rect.x, b.rect.y, b.rect.width, b.rect.height, b.color, true);
        const int32_t tw = static_cast<int32_t>(b.label.size()) * 7;
        rend.draw_text(b.label, b.rect.x + (b.rect.width - tw) / 2, b.rect.y + 6, sf::Color::White, 12);
    }
}

bool quest_dialog::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    if (!visible_)
        return false;

    if (btn == sf::Mouse::Button::Left)
    {
        for (const auto& b : buttons_for_selected())
        {
            if (!b.rect.contains(x, y))
                continue;
            const auto* q = selected();
            switch (b.action)
            {
            case 1:
                if (q && on_accept_)
                    on_accept_(npc_entity_id_, q->quest_id);
                break;
            case 2:
                if (q && on_complete_)
                    on_complete_(npc_entity_id_, q->quest_id);
                break;
            case 3:
                if (q && on_abandon_)
                    on_abandon_(q->quest_id);
                break;
            default:
                close();
                break;
            }
            return true;
        }

        const int32_t list_x = bounds_.x + 8;
        const int32_t list_y = bounds_.y + list_top + 18;
        if (x >= list_x && x < list_x + list_width && y >= list_y)
        {
            const size_t idx = static_cast<size_t>((y - list_y) / row_height);
            if (idx < quests_.size())
            {
                selected_index_ = idx;
                return true;
            }
        }
    }

    return dialog::handle_mouse_down(x, y, btn);
}

bool quest_dialog::handle_mouse_move(int32_t x, int32_t y)
{
    if (!visible_)
        return false;
    hovered_index_.reset();
    const int32_t list_x = bounds_.x + 8;
    const int32_t list_y = bounds_.y + list_top + 18;
    if (x >= list_x && x < list_x + list_width && y >= list_y)
    {
        const size_t idx = static_cast<size_t>((y - list_y) / row_height);
        if (idx < quests_.size())
            hovered_index_ = idx;
    }
    return dialog::handle_mouse_move(x, y);
}

} // namespace hb
