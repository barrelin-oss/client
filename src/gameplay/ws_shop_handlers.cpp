// Shops and the bank over the JSON protocol (docs/protocol/npc.md). The catalogue and the
// bank contents come inside player_interact_response; buying, selling (quote, then
// confirm) and deposit/withdraw are request/response, and the inventory pushes that
// follow keep the bag in sync. After a deposit or withdrawal the bank is re-read by
// interacting again, which is how the server hands out its contents.

#include "gameplay/ws_message_handler.hpp"
#include "gameplay/game_state.hpp"
#include "network/messages/shop.hpp"
#include "ui/dialogs/shop_dialog.hpp"
#include "ui/dialogs/bank_dialog.hpp"
#include <algorithm>
#include <format>
#include <spdlog/spdlog.h>

namespace hb
{

namespace
{

shop_dialog* get_shop_dialog(game_state_manager& game)
{
    return dynamic_cast<shop_dialog*>(game.ui().get_dialog(dialog_type::shop));
}

shop_sell_dialog* get_sell_dialog(game_state_manager& game)
{
    return dynamic_cast<shop_sell_dialog*>(game.ui().get_dialog(dialog_type::shop_sell));
}

bank_dialog* get_bank_dialog(game_state_manager& game)
{
    return dynamic_cast<bank_dialog*>(game.ui().get_dialog(dialog_type::bank));
}

} // namespace

// ---------------------------------------------------------------------------
// Shop
// ---------------------------------------------------------------------------

void ws_message_handler::open_shop(uint32_t npc_entity_id, const json& interaction_data)
{
    shop_npc_id_ = npc_entity_id;
    shop_catalogue_.clear();

    std::vector<shop_item> rows;
    if (interaction_data.contains("items") && interaction_data["items"].is_array())
    {
        for (const auto& o : interaction_data["items"])
        {
            auto c = shop_catalogue_item::from_object(o);
            shop_catalogue_.push_back(c.template_id);
            shop_item row;
            row.item_data.template_id = c.template_id;
            row.item_data.name = c.name;
            row.item_data.price = c.base_price;
            row.item_data.level_req = static_cast<uint16_t>(std::max(0, c.level_limit));
            row.item_data.count = static_cast<uint32_t>(std::max(1, c.count));
            row.price = c.price;
            rows.push_back(std::move(row));
        }
    }

    auto* dlg = get_shop_dialog(*game_);
    if (!dlg)
        return;
    // Shops name themselves by shop_type (the merchant's name); npc_name is the dialog/bank key
    std::string shop_name = json_text(interaction_data, "npc_name");
    if (shop_name.empty())
        shop_name = json_text(interaction_data, "shop_type");
    dlg->set_shop_name(shop_name);
    dlg->set_items(rows);
    // Side by side: catalogue on the left, the bag to sell on the right. Neither is modal:
    // a modal front dialog swallows every click, including the ones meant for the other
    dlg->set_bounds({60, 110, 350, 360});
    dlg->set_modal(false);
    dlg->set_player_gold(static_cast<uint32_t>(std::max<int64_t>(0, game_->inventory().gold())));
    if (!dlg->is_open())
        game_->ui().open_dialog(dialog_type::shop);

    refresh_sell_dialog();
    if (auto* sell = get_sell_dialog(*game_))
    {
        sell->set_bounds({430, 140, 300, 300});
        sell->set_modal(false);
        if (!sell->is_open())
            game_->ui().open_dialog(dialog_type::shop_sell);
    }

    spdlog::info("Shop '{}' opened: {} items", shop_name, rows.size());
}

// The sell list is the bag as it is now. The value column is the merchant's own offer:
// a quote (shop_sell_request) is asked for every item and commits to nothing, since only
// shop_sell_confirm_request sells. Until the quote arrives the value is 0; an item the
// merchant refuses leaves the list.
void ws_message_handler::refresh_sell_dialog()
{
    auto* sell = get_sell_dialog(*game_);
    if (!sell)
        return;
    sell_rows_.clear();
    sell_quote_seqs_.clear();
    game_->inventory().for_each_bag_item(
        [&](uint32_t item_id, const bag_item& entry)
        {
            shop_sell_dialog::sell_item row;
            row.item_ptr = &entry.data;
            row.inventory_slot = static_cast<int32_t>(item_id); // v2 items have ids, not slots
            row.sell_price = 0;
            sell_rows_.push_back(row);
        });
    sell->set_items(sell_rows_);
    if (shop_npc_id_ == 0)
        return;
    for (const auto& row : sell_rows_)
    {
        auto msg = make_shop_sell_request(shop_npc_id_, static_cast<uint32_t>(row.inventory_slot), 1);
        sell_quote_seqs_[msg.value("seq", 0u)] = static_cast<uint32_t>(row.inventory_slot);
        game_->ws_connection().send(msg);
    }
}

void ws_message_handler::request_shop_buy(size_t catalogue_index, uint32_t count)
{
    if (shop_npc_id_ == 0 || catalogue_index >= shop_catalogue_.size())
        return;
    const uint32_t template_id = shop_catalogue_[catalogue_index];
    game_->ws_connection().send(make_shop_buy_request(shop_npc_id_, template_id, static_cast<int32_t>(count)));
    spdlog::info("Sent shop_buy_request: npc={} template={} count={}", shop_npc_id_, template_id, count);
}

void ws_message_handler::request_shop_sell(uint32_t item_id, uint32_t count)
{
    if (shop_npc_id_ == 0 || item_id == 0)
        return;
    pending_sell_item_id_ = item_id;
    pending_sell_count_ = static_cast<int32_t>(std::max(1u, count));
    auto msg = make_shop_sell_request(shop_npc_id_, item_id, pending_sell_count_);
    pending_sell_seq_ = msg.value("seq", 0u);
    game_->ws_connection().send(msg);
    spdlog::info("Sent shop_sell_request: npc={} item={} count={}", shop_npc_id_, item_id, pending_sell_count_);
}

void ws_message_handler::handle_shop_buy_response(const json& message)
{
    auto r = shop_buy_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event("Cannot buy: " + (r.error.empty() ? "refused" : r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event(std::format("Bought {} x{} for {} gold.", r.item_name, r.count, r.price_paid),
                                      message_color::green);
    if (auto* dlg = get_shop_dialog(*game_); dlg && r.gold_remaining >= 0)
        dlg->set_player_gold(static_cast<uint32_t>(r.gold_remaining));
}

void ws_message_handler::handle_shop_sell_response(const json& message)
{
    auto r = shop_sell_response_data::from_json(message);
    const uint32_t seq = message.value("seq", 0u);

    // A quote asked for the list: fill in the value, or drop what the merchant refuses
    if (auto it = sell_quote_seqs_.find(seq); it != sell_quote_seqs_.end())
    {
        const uint32_t item_id = it->second;
        sell_quote_seqs_.erase(it);
        auto row = std::find_if(sell_rows_.begin(), sell_rows_.end(),
                                [&](const auto& s) { return static_cast<uint32_t>(s.inventory_slot) == item_id; });
        if (row != sell_rows_.end())
        {
            if (r.success)
                row->sell_price = static_cast<uint32_t>(std::max<int64_t>(0, r.offered_price));
            else
                sell_rows_.erase(row);
            if (auto* sell = get_sell_dialog(*game_))
                sell->set_items(sell_rows_);
        }
        return;
    }

    if (seq != pending_sell_seq_ || !r.success || pending_sell_item_id_ == 0)
    {
        game_->get_status_log().add_event("Cannot sell: " + (r.error.empty() ? "refused" : r.error), message_color::red);
        pending_sell_item_id_ = 0;
        return;
    }
    // The quote is accepted as-is: the sell dialog already asked the player once
    game_->get_status_log().add_event(std::format("{}: the merchant offers {} gold.", r.item_name, r.offered_price),
                                      message_color::blue);
    game_->ws_connection().send(make_shop_sell_confirm_request(shop_npc_id_, pending_sell_item_id_, pending_sell_count_));
}

void ws_message_handler::handle_shop_sell_confirm_response(const json& message)
{
    auto r = shop_sell_confirm_response_data::from_json(message);
    pending_sell_item_id_ = 0;
    if (!r.success)
    {
        game_->get_status_log().add_event("Sale failed: " + (r.error.empty() ? "refused" : r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event(std::format("Sold for {} gold.", r.gold_received), message_color::green);
    if (auto* dlg = get_shop_dialog(*game_); dlg && r.gold_total >= 0)
        dlg->set_player_gold(static_cast<uint32_t>(r.gold_total));
    refresh_sell_dialog();
}

// ---------------------------------------------------------------------------
// Bank
// ---------------------------------------------------------------------------

void ws_message_handler::open_bank(uint32_t npc_entity_id, const json& interaction_data)
{
    bank_npc_id_ = npc_entity_id;
    auto* dlg = get_bank_dialog(*game_);
    if (!dlg)
        return;
    dlg->set_modal(false); // the inventory must stay clickable to drag items into the bank

    // The dialog keeps pointers into bank_items_, so the vector is sized once and never
    // reallocated while the dialog is open
    bank_items_.assign(bank_dialog::max_slots, item{});
    for (int32_t slot = 0; slot < bank_dialog::max_slots; ++slot)
        dlg->clear_slot(slot);

    size_t occupied = 0;
    if (interaction_data.contains("items") && interaction_data["items"].is_array())
    {
        for (const auto& o : interaction_data["items"])
        {
            auto b = bank_slot_item::from_object(o);
            if (b.page != 0 || b.slot < 0 || b.slot >= bank_dialog::max_slots || b.item_id == 0)
                continue; // one page in this dialog
            item& it = bank_items_[static_cast<size_t>(b.slot)];
            it.item_id = b.item_id;
            it.name = b.name;
            it.count = b.count;
            it.durability = b.durability;
            it.max_durability = b.max_durability;
            dlg->set_item(b.slot, &it);
            ++occupied;
        }
    }
    if (interaction_data.contains("gold"))
        dlg->set_stored_gold(interaction_data.value("gold", 0u));
    if (!dlg->is_open())
        game_->ui().open_dialog(dialog_type::bank);
    spdlog::info("Bank '{}' opened: {} items", json_text(interaction_data, "npc_name"), occupied);
}

void ws_message_handler::request_bank_deposit(uint32_t item_id)
{
    if (bank_npc_id_ == 0 || item_id == 0)
        return;
    game_->ws_connection().send(make_bank_deposit_request(bank_npc_id_, item_id));
    spdlog::info("Sent bank_deposit_request: npc={} item={}", bank_npc_id_, item_id);
}

void ws_message_handler::request_bank_withdraw(int32_t bank_slot)
{
    if (bank_npc_id_ == 0)
        return;
    game_->ws_connection().send(make_bank_withdraw_request(bank_npc_id_, 0, bank_slot));
    spdlog::info("Sent bank_withdraw_request: npc={} slot={}", bank_npc_id_, bank_slot);
}

void ws_message_handler::handle_bank_deposit_response(const json& message)
{
    auto r = bank_action_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event("Deposit refused: " + (r.error.empty() ? "refused" : r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event("Deposited " + r.item_name + ".", message_color::green);
    if (bank_npc_id_ != 0)
        request_interact(bank_npc_id_); // re-read the bank
}

void ws_message_handler::handle_bank_withdraw_response(const json& message)
{
    auto r = bank_action_response_data::from_json(message);
    if (!r.success)
    {
        game_->get_status_log().add_event("Withdrawal refused: " + (r.error.empty() ? "refused" : r.error), message_color::red);
        return;
    }
    game_->get_status_log().add_event("Withdrew " + r.item_name + ".", message_color::green);
    if (bank_npc_id_ != 0)
        request_interact(bank_npc_id_);
}

} // namespace hb
