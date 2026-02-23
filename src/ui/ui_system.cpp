#include "ui/ui_system.hpp"
#include "ui/dialog_manager.hpp"
#include "ui/dialogs/dialogs.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include "gameplay/item.hpp"
#include "gameplay/item_format.hpp"
#include "world/ground_item.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb
{

// ui_system constructor/destructor (defined here due to unique_ptr to forward-declared type)
ui_system::ui_system() = default;
ui_system::~ui_system() = default;

// dialog base class implementation is in dialog_base.cpp

// ui_system implementation

void ui_system::initialize()
{
    dialog_manager_ = std::make_unique<dialog_manager>();
    dialog_manager_->initialize(sprites_);
    spdlog::info("UI system initialized");
}

void ui_system::shutdown()
{
    if (dialog_manager_)
    {
        dialog_manager_->shutdown();
        dialog_manager_.reset();
    }
    yaml_icon_panel_ = nullptr;
    dialogs_.clear();
    dialog_order_.clear();
    focused_ = nullptr;
    spdlog::info("UI system shutdown");
}

void ui_system::update(float delta_time, const input& inp)
{
    text_input_active_ = false;

    // Update legacy dialogs
    for (auto* dlg : dialog_order_)
    {
        dlg->update(delta_time, inp);
    }

    // Update data-driven dialogs
    if (dialog_manager_)
    {
        dialog_manager_->update(delta_time, inp);
    }

    int32_t mx = inp.mouse_x();
    int32_t my = inp.mouse_y();

    // Update drag position every frame
    if (drag_state_.active)
    {
        update_drag_position(mx, my);
    }

    // Route mouse move to data-driven dialogs (for hover states)
    if (dialog_manager_)
    {
        dialog_manager_->handle_mouse_move(mx, my);
    }

    // Route mouse move to legacy dialogs (for drag and hover states)
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
    {
        if ((*it)->is_open() && (*it)->handle_mouse_move(mx, my))
        {
            break;
        }
    }

    // Route mouse clicks to open dialogs (front to back)
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left))
    {
        bool handled = false;
        // First check data-driven dialogs (they render on top)
        if (dialog_manager_ && dialog_manager_->handle_mouse_down(mx, my, sf::Mouse::Button::Left))
        {
            handled = true;
        }
        else
        {
            // Then check legacy dialogs
            for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
            {
                if ((*it)->is_open())
                {
                    // For modal dialogs, always send the click (even if outside bounds)
                    // The dialog will handle blocking clicks outside its area
                    if ((*it)->modal())
                    {
                        (*it)->handle_mouse_down(mx, my, sf::Mouse::Button::Left);
                        handled = true;
                        break; // Modal consumes all clicks
                    }
                    // For non-modal, only if click is inside bounds
                    if ((*it)->bounds().contains(mx, my))
                    {
                        dialog* dlg = *it; // Save pointer before modifying vector
                        bring_to_front(dlg);
                        dlg->handle_mouse_down(mx, my, sf::Mouse::Button::Left);
                        handled = true;
                        break;
                    }
                }
            }
        }
        // Mark consumed immediately so the game world won't process this click,
        // even if the dialog closes itself in the click handler
        if (handled)
        {
            mouse_consumed_left_ = true;
        }
    }

    // Right-click closes the topmost right-click-closeable dialog
    if (inp.is_mouse_pressed(sf::Mouse::Button::Right))
    {
        bool closed_any = false;

        // Check data-driven dialogs first (rendered on top)
        if (dialog_manager_)
        {
            // Iterate dialog order in reverse (front to back) via the internal order
            // We use find_dialog + close_dialog since dialog_manager doesn't expose order
            // Instead, check if click is over any managed dialog and close it
            if (auto* dlg = dialog_manager_->find_topmost_at(mx, my))
            {
                if (dlg->right_click_closeable())
                {
                    dlg->close();
                    closed_any = true;
                }
            }
        }

        // Then check legacy dialogs
        if (!closed_any)
        {
            for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
            {
                if ((*it)->is_open() && (*it)->bounds().contains(mx, my))
                {
                    if ((*it)->right_click_closeable())
                    {
                        (*it)->close();
                        closed_any = true;
                    }
                    break; // Only close topmost
                }
            }
        }

        if (closed_any)
        {
            mouse_consumed_right_ = true;
        }
    }

    // Route mouse release to dialogs
    if (inp.is_mouse_released(sf::Mouse::Button::Left))
    {
        // Resolve item drag first
        if (drag_state_.active)
        {
            bool shift = inp.is_key_down(sf::Keyboard::Key::LShift) ||
                         inp.is_key_down(sf::Keyboard::Key::RShift);
            end_item_drag(mx, my, shift);
        }
        else
        {
            // First try data-driven dialogs
            if (dialog_manager_)
            {
                dialog_manager_->handle_mouse_up(mx, my, sf::Mouse::Button::Left);
            }
            // Then legacy dialogs - copy list since callbacks may modify it
            auto dialogs_copy = dialog_order_;
            for (auto it = dialogs_copy.rbegin(); it != dialogs_copy.rend(); ++it)
            {
                if ((*it)->is_open())
                {
                    (*it)->handle_mouse_up(mx, my, sf::Mouse::Button::Left);
                    if ((*it)->modal())
                    {
                        break; // Modal consumes the event
                    }
                }
            }
        }
    }

    // Route mouse wheel to data-driven dialogs
    int32_t wheel_delta = inp.wheel_delta();
    if (wheel_delta != 0 && dialog_manager_)
    {
        dialog_manager_->handle_mouse_wheel(mx, my, wheel_delta);
    }

    // Route key presses to data-driven dialogs first
    if (dialog_manager_)
    {
        if (inp.is_key_pressed(sf::Keyboard::Key::Escape))
        {
            if (dialog_manager_->handle_key_press(sf::Keyboard::Key::Escape))
            {
                return;
            }
        }
        if (inp.is_key_pressed(sf::Keyboard::Key::Enter))
        {
            if (dialog_manager_->handle_key_press(sf::Keyboard::Key::Enter))
            {
                return;
            }
        }
    }

    // Route key presses to legacy dialogs (front to back)
    // This ensures modal dialogs like connection_dialog can receive Escape key
    static constexpr sf::Keyboard::Key routed_keys[] = {
        sf::Keyboard::Key::Escape,
        sf::Keyboard::Key::Enter,
        sf::Keyboard::Key::Backspace,
        sf::Keyboard::Key::Delete,
        sf::Keyboard::Key::Left,
        sf::Keyboard::Key::Right,
        sf::Keyboard::Key::Home,
        sf::Keyboard::Key::End,
        sf::Keyboard::Key::PageUp,
        sf::Keyboard::Key::PageDown,
    };
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
    {
        if ((*it)->is_open())
        {
            for (auto key : routed_keys)
            {
                if (inp.is_key_pressed(key))
                {
                    if ((*it)->handle_key_press(key))
                    {
                        return; // Key was consumed
                    }
                }
            }
            // Modal dialogs should block further key routing
            if ((*it)->modal())
            {
                break;
            }
        }
    }

    // Escape not consumed by any dialog — toggle options (playing state only)
    if (inp.is_key_pressed(sf::Keyboard::Key::Escape))
    {
        auto* icon_panel = get_dialog(dialog_type::icon_panel);
        if (icon_panel && icon_panel->is_open())
        {
            toggle_dialog(dialog_type::options);
            return;
        }
    }

    // Route text input to legacy dialogs (front to back)
    auto text = inp.text_input();
    if (!text.empty())
    {
        for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
        {
            if ((*it)->is_open())
            {
                bool consumed = false;
                for (char ch : text)
                {
                    if ((*it)->handle_text_input(static_cast<char32_t>(ch)))
                    {
                        consumed = true;
                    }
                }
                if (consumed)
                {
                    text_input_active_ = true;
                    return;
                }
                if ((*it)->modal())
                    break;
            }
        }
    }
}

void ui_system::render(renderer& rend)
{
    // Render legacy dialogs in order (back to front)
    for (auto* dlg : dialog_order_)
    {
        if (dlg->is_open())
        {
            dlg->render(rend);
        }
    }

    // Render data-driven dialogs
    if (dialog_manager_)
    {
        dialog_manager_->render(rend);
    }

    // Render tooltip
    if (tooltip_visible_ && !tooltip_text_.empty())
    {
        int32_t tooltip_width = static_cast<int32_t>(tooltip_text_.length() * 7 + 8);
        int32_t tooltip_height = 20;

        rend.draw_rect(tooltip_x_, tooltip_y_, tooltip_width, tooltip_height, sf::Color(40, 40, 50, 240), true);
        rend.draw_rect(tooltip_x_, tooltip_y_, tooltip_width, tooltip_height, sf::Color(100, 100, 120), false);
        rend.draw_text(tooltip_text_, tooltip_x_ + 4, tooltip_y_ + 3, sf::Color::White);
    }

    // Render dragged item info tooltip (inventory items move in-place, no separate sprite)
    if (drag_state_.active && drag_state_.held_item)
    {
        render_held_item_info(rend, *drag_state_.held_item,
                              drag_state_.cursor_x + 16, drag_state_.cursor_y + 16);
    }
}

bool ui_system::handle_mouse_move(int32_t x, int32_t y)
{
    // First try data-driven dialogs
    if (dialog_manager_ && dialog_manager_->handle_mouse_move(x, y))
    {
        return true;
    }

    // Then legacy dialogs (process in reverse order, front to back)
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
    {
        if ((*it)->is_open() && (*it)->handle_mouse_move(x, y))
        {
            return true;
        }
    }
    return false;
}

bool ui_system::handle_mouse_down(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    // First try data-driven dialogs (they render on top)
    if (dialog_manager_ && dialog_manager_->handle_mouse_down(x, y, btn))
    {
        return true;
    }

    // Check if clicking outside modal dialog
    if (is_modal_open())
    {
        for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
        {
            if ((*it)->is_open() && (*it)->modal())
            {
                if (!(*it)->bounds().contains(x, y))
                {
                    return true; // Block input outside modal
                }
                break;
            }
        }
    }

    // Process legacy dialogs
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
    {
        if ((*it)->is_open())
        {
            if ((*it)->bounds().contains(x, y))
            {
                dialog* dlg = *it; // Save pointer before modifying vector
                bring_to_front(dlg);
                dlg->handle_mouse_down(x, y, btn);
                return true;
            }
        }
    }

    // Clear focus if clicking outside
    if (focused_)
    {
        focused_->set_focused(false);
        focused_ = nullptr;
    }

    return false;
}

bool ui_system::handle_mouse_up(int32_t x, int32_t y, sf::Mouse::Button btn)
{
    // First try data-driven dialogs
    if (dialog_manager_ && dialog_manager_->handle_mouse_up(x, y, btn))
    {
        return true;
    }

    // Then legacy dialogs
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
    {
        if ((*it)->is_open() && (*it)->handle_mouse_up(x, y, btn))
        {
            return true;
        }
    }
    return false;
}

bool ui_system::handle_key_press(sf::Keyboard::Key key)
{
    // First try data-driven dialogs
    if (dialog_manager_ && dialog_manager_->handle_key_press(key))
    {
        return true;
    }

    // Handle escape to close top legacy dialog
    if (key == sf::Keyboard::Key::Escape)
    {
        for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
        {
            if ((*it)->is_open() && (*it)->closeable())
            {
                (*it)->close();
                return true;
            }
        }
    }

    if (focused_)
    {
        return focused_->handle_key_press(key);
    }

    // Route to open legacy dialogs (front to back)
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
    {
        if ((*it)->is_open() && (*it)->handle_key_press(key))
        {
            return true;
        }
    }

    return false;
}

bool ui_system::handle_text_input(char32_t unicode)
{
    // First try data-driven dialogs
    if (dialog_manager_ && dialog_manager_->handle_text_input(unicode))
    {
        return true;
    }

    if (focused_)
    {
        return focused_->handle_text_input(unicode);
    }

    // Route to open legacy dialogs (front to back)
    for (auto it = dialog_order_.rbegin(); it != dialog_order_.rend(); ++it)
    {
        if ((*it)->is_open() && (*it)->handle_text_input(unicode))
        {
            return true;
        }
    }

    return false;
}

dialog* ui_system::get_dialog(dialog_type type)
{
    // Special case: icon_panel may be managed by dialog_manager (YAML version)
    if (type == dialog_type::icon_panel && yaml_icon_panel_)
    {
        return yaml_icon_panel_;
    }

    auto it = dialogs_.find(type);
    if (it != dialogs_.end())
    {
        return it->second.get();
    }
    return nullptr;
}

icon_panel_interface* ui_system::get_icon_panel()
{
    if (yaml_icon_panel_)
        return yaml_icon_panel_;
    auto it = dialogs_.find(dialog_type::icon_panel);
    if (it != dialogs_.end())
        return dynamic_cast<icon_panel_interface*>(it->second.get());
    return nullptr;
}

void ui_system::add_dialog(dialog_type type, std::unique_ptr<dialog> dlg)
{
    // Remove existing dialog of this type if present
    if (auto it = dialogs_.find(type); it != dialogs_.end())
    {
        dialog* old_ptr = it->second.get();
        clear_focus_if_owned_by(old_ptr);
        dialog_order_.erase(std::remove(dialog_order_.begin(), dialog_order_.end(), old_ptr), dialog_order_.end());
        dialogs_.erase(it);
    }

    dialog* ptr = dlg.get();
    dialogs_[type] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::open_dialog(dialog_type type)
{
    // Special case: icon_panel may be managed by dialog_manager (YAML version)
    if (type == dialog_type::icon_panel && yaml_icon_panel_)
    {
        yaml_icon_panel_->open();
        return;
    }

    auto* dlg = get_dialog(type);
    if (dlg)
    {
        dlg->open();
        bring_to_front(dlg);
    }
}

void ui_system::close_dialog(dialog_type type)
{
    auto* dlg = get_dialog(type);
    if (dlg)
    {
        dlg->close();
    }
}

void ui_system::toggle_dialog(dialog_type type)
{
    auto* dlg = get_dialog(type);
    if (dlg)
    {
        if (dlg->is_open())
        {
            dlg->close();
        }
        else
        {
            dlg->open();
            bring_to_front(dlg);
        }
    }
}

bool ui_system::is_dialog_open(dialog_type type) const
{
    auto it = dialogs_.find(type);
    if (it != dialogs_.end())
    {
        return it->second->is_open();
    }
    return false;
}

void ui_system::close_all_dialogs()
{
    // Close legacy dialogs
    for (auto& [type, dlg] : dialogs_)
    {
        dlg->close();
    }

    // Close YAML-based icon panel if it exists
    if (yaml_icon_panel_)
    {
        yaml_icon_panel_->close();
    }

    // Close all data-driven dialogs
    if (dialog_manager_)
    {
        dialog_manager_->close_all();
    }
}

void ui_system::create_login_dialog()
{
    add_dialog(dialog_type::login, std::make_unique<login_dialog>());
}

void ui_system::create_character_select_dialog()
{
    add_dialog(dialog_type::character_select, std::make_unique<character_select_dialog>());
}

void ui_system::create_character_dialog()
{
    add_dialog(dialog_type::character_info, std::make_unique<character_dialog>());
}

void ui_system::create_inventory_dialog()
{
    auto dlg = std::make_unique<inventory_dialog>();
    dlg->set_sprite_manager(sprites_);
    add_dialog(dialog_type::inventory, std::move(dlg));
}

void ui_system::create_equipment_dialog()
{
    add_dialog(dialog_type::equipment, std::make_unique<equipment_dialog>());
}

void ui_system::create_spellbook_dialog()
{
    add_dialog(dialog_type::spellbook, std::make_unique<spellbook_dialog>());
}

void ui_system::create_skills_dialog()
{
    add_dialog(dialog_type::skills, std::make_unique<skills_dialog>());
}

void ui_system::create_shop_dialog()
{
    add_dialog(dialog_type::shop, std::make_unique<shop_dialog>());
}

void ui_system::create_bank_dialog()
{
    add_dialog(dialog_type::bank, std::make_unique<bank_dialog>());
}

void ui_system::create_party_dialog()
{
    add_dialog(dialog_type::party, std::make_unique<party_dialog>());
}

void ui_system::create_guild_dialog()
{
    add_dialog(dialog_type::guild, std::make_unique<guild_dialog>());
}

void ui_system::create_npc_dialog()
{
    add_dialog(dialog_type::npc_dialog, std::make_unique<npc_dialog>());
}

void ui_system::create_chat_dialog()
{
    auto dlg = std::make_unique<chat_dialog>();
    dlg->set_draggable(true);
    dlg->set_closeable(true);
    add_dialog(dialog_type::chat, std::move(dlg));
}

void ui_system::create_options_dialog()
{
    auto dlg = std::make_unique<settings_dialog>();
    dlg->set_ui_style(style_);
    add_dialog(dialog_type::options, std::move(dlg));
}

void ui_system::create_trade_dialog()
{
    add_dialog(dialog_type::trade, std::make_unique<trade_dialog>());
}

void ui_system::create_craft_dialog()
{
    add_dialog(dialog_type::manufacture, std::make_unique<craft_dialog>());
}

void ui_system::create_map_dialog()
{
    add_dialog(dialog_type::map, std::make_unique<map_dialog>());
}

void ui_system::create_repair_dialog()
{
    add_dialog(dialog_type::repair, std::make_unique<repair_dialog>());
}

void ui_system::create_help_dialog()
{
    add_dialog(dialog_type::help, std::make_unique<help_dialog>());
}

void ui_system::create_message_box(std::string_view title, std::string_view message, std::function<void()> on_ok)
{
    auto dlg = std::make_unique<dialog>(dialog_type::message_box);
    dlg->set_title(title);
    dlg->set_bounds(
        {static_cast<int32_t>(screen_width) / 2 - 150, static_cast<int32_t>(screen_height) / 2 - 75, 300, 150});
    dlg->set_modal(true);

    // Message (positioned below title bar)
    auto msg_label = std::make_unique<ui_label>();
    msg_label->set_bounds({20, 35, 260, 60});
    msg_label->set_text(message);
    msg_label->set_alignment(ui_label::alignment::center);
    dlg->add_child(std::move(msg_label));

    // OK button
    auto ok_btn = std::make_unique<ui_button>();
    ok_btn->set_bounds({110, 110, 80, 28});
    ok_btn->set_text("OK");
    ok_btn->set_on_click(
        [this, on_ok]()
        {
            close_dialog(dialog_type::message_box);
            if (on_ok)
                on_ok();
        });
    dlg->add_child(std::move(ok_btn));

    add_dialog(dialog_type::message_box, std::move(dlg));
    get_dialog(dialog_type::message_box)->open();
}

void ui_system::create_confirm_box(std::string_view title,
                                   std::string_view message,
                                   std::function<void(bool)> on_result)
{
    auto dlg = std::make_unique<dialog>(dialog_type::confirm);
    dlg->set_title(title);
    dlg->set_bounds(
        {static_cast<int32_t>(screen_width) / 2 - 150, static_cast<int32_t>(screen_height) / 2 - 75, 300, 150});
    dlg->set_modal(true);

    // Message (positioned below title bar at y=24)
    auto msg_label = std::make_unique<ui_label>();
    msg_label->set_bounds({20, 35, 260, 60});
    msg_label->set_text(message);
    msg_label->set_alignment(ui_label::alignment::center);
    dlg->add_child(std::move(msg_label));

    // Yes button
    auto yes_btn = std::make_unique<ui_button>();
    yes_btn->set_bounds({60, 110, 80, 28});
    yes_btn->set_text("Yes");
    yes_btn->set_on_click(
        [this, on_result]()
        {
            close_dialog(dialog_type::confirm);
            if (on_result)
                on_result(true);
        });
    dlg->add_child(std::move(yes_btn));

    // No button
    auto no_btn = std::make_unique<ui_button>();
    no_btn->set_bounds({160, 110, 80, 28});
    no_btn->set_text("No");
    no_btn->set_on_click(
        [this, on_result]()
        {
            close_dialog(dialog_type::confirm);
            if (on_result)
                on_result(false);
        });
    dlg->add_child(std::move(no_btn));

    add_dialog(dialog_type::confirm, std::move(dlg));
    get_dialog(dialog_type::confirm)->open();
}

void ui_system::create_drop_confirm_box(const item& itm,
                                        std::function<void(bool confirmed, bool skip_next)> on_result)
{
    auto info_lines = build_item_info(itm);

    // Calculate dialog height based on content:
    // title bar ~24px + item info lines + question label + toggle label + buttons + padding
    int32_t line_height = 16;
    int32_t content_y = 32; // Start below title bar
    int32_t info_height = static_cast<int32_t>(info_lines.size()) * line_height;
    int32_t total_height = content_y + info_height + 10 + line_height + 10 + 22 + 10 + 28 + 10;

    auto dlg = std::make_unique<dialog>(dialog_type::confirm);
    dlg->set_title("Drop Item");
    dlg->set_bounds({static_cast<int32_t>(screen_width) / 2 - 140,
                     static_cast<int32_t>(screen_height) / 2 - total_height / 2, 280, total_height});
    dlg->set_modal(true);

    // Item info lines
    int32_t y_pos = content_y;
    for (const auto& line : info_lines)
    {
        auto lbl = std::make_unique<ui_label>();
        lbl->set_bounds({20, y_pos, 240, line_height});
        lbl->set_text(line.text);
        lbl->set_text_color(line.color);
        dlg->add_child(std::move(lbl));
        y_pos += line_height;
    }

    y_pos += 10;

    // Question label
    auto question = std::make_unique<ui_label>();
    question->set_bounds({20, y_pos, 240, line_height});
    question->set_text("Drop this item?");
    question->set_alignment(ui_label::alignment::center);
    dlg->add_child(std::move(question));
    y_pos += line_height + 10;

    // "Don't ask again" toggle (button styled as text toggle)
    auto skip_toggle = std::make_shared<bool>(false);
    auto toggle_btn = std::make_unique<ui_button>();
    toggle_btn->set_bounds({20, y_pos, 240, 22});
    toggle_btn->set_text("Don't ask for same item: OFF");
    toggle_btn->set_normal_color(sf::Color(40, 40, 50, 180));
    toggle_btn->set_hover_color(sf::Color(50, 50, 65, 200));
    auto* toggle_ptr = toggle_btn.get();
    toggle_btn->set_on_click(
        [toggle_ptr, skip_toggle]()
        {
            *skip_toggle = !*skip_toggle;
            if (*skip_toggle)
            {
                toggle_ptr->set_text("Don't ask for same item: ON");
                toggle_ptr->set_normal_color(sf::Color(30, 60, 30, 200));
                toggle_ptr->set_hover_color(sf::Color(40, 70, 40, 220));
            }
            else
            {
                toggle_ptr->set_text("Don't ask for same item: OFF");
                toggle_ptr->set_normal_color(sf::Color(40, 40, 50, 180));
                toggle_ptr->set_hover_color(sf::Color(50, 50, 65, 200));
            }
        });
    dlg->add_child(std::move(toggle_btn));
    y_pos += 22 + 10;

    // Yes button
    auto yes_btn = std::make_unique<ui_button>();
    yes_btn->set_bounds({50, y_pos, 80, 28});
    yes_btn->set_text("Yes");
    yes_btn->set_on_click(
        [this, on_result, skip_toggle]()
        {
            close_dialog(dialog_type::confirm);
            if (on_result)
                on_result(true, *skip_toggle);
        });
    dlg->add_child(std::move(yes_btn));

    // No button
    auto no_btn = std::make_unique<ui_button>();
    no_btn->set_bounds({150, y_pos, 80, 28});
    no_btn->set_text("No");
    no_btn->set_on_click(
        [this, on_result]()
        {
            close_dialog(dialog_type::confirm);
            if (on_result)
                on_result(false, false);
        });
    dlg->add_child(std::move(no_btn));

    add_dialog(dialog_type::confirm, std::move(dlg));
    get_dialog(dialog_type::confirm)->open();
}

void ui_system::create_input_box(std::string_view title,
                                 std::string_view prompt,
                                 std::function<void(std::string_view)> on_submit)
{
    auto dlg = std::make_unique<dialog>(dialog_type::input_box);
    dlg->set_title(title);
    dlg->set_bounds(
        {static_cast<int32_t>(screen_width) / 2 - 150, static_cast<int32_t>(screen_height) / 2 - 70, 300, 140});
    dlg->set_modal(true);

    // Prompt
    auto prompt_label = std::make_unique<ui_label>();
    prompt_label->set_bounds({20, 40, 260, 20});
    prompt_label->set_text(prompt);
    dlg->add_child(std::move(prompt_label));

    // Input
    auto text_input = std::make_unique<ui_text_input>();
    text_input->set_id("input_field");
    text_input->set_bounds({20, 65, 260, 24});
    dlg->add_child(std::move(text_input));

    // OK button
    auto ok_btn = std::make_unique<ui_button>();
    ok_btn->set_bounds({60, 100, 80, 28});
    ok_btn->set_text("OK");
    ok_btn->set_on_click(
        [this, on_submit]()
        {
            auto* dlg = get_dialog(dialog_type::input_box);
            if (dlg)
            {
                if (auto* input = dlg->find_child("input_field"))
                {
                    auto* text_input = static_cast<ui_text_input*>(input);
                    if (on_submit)
                        on_submit(text_input->text());
                }
            }
            close_dialog(dialog_type::input_box);
        });
    dlg->add_child(std::move(ok_btn));

    // Cancel button
    auto cancel_btn = std::make_unique<ui_button>();
    cancel_btn->set_bounds({160, 100, 80, 28});
    cancel_btn->set_text("Cancel");
    cancel_btn->set_on_click([this]() { close_dialog(dialog_type::input_box); });
    dlg->add_child(std::move(cancel_btn));

    // Remove old input box (remove from dialog_order_ before erasing from dialogs_)
    if (auto it = dialogs_.find(dialog_type::input_box); it != dialogs_.end())
    {
        dialog* old_ptr = it->second.get();
        clear_focus_if_owned_by(old_ptr);
        dialog_order_.erase(std::remove(dialog_order_.begin(), dialog_order_.end(), old_ptr), dialog_order_.end());
        dialogs_.erase(it);
    }

    dialog* ptr = dlg.get();
    dialogs_[dialog_type::input_box] = std::move(dlg);
    dialog_order_.push_back(ptr);
    ptr->open();
}

void ui_system::show_tooltip(std::string_view text, int32_t x, int32_t y)
{
    tooltip_text_ = text;
    tooltip_x_ = x;
    tooltip_y_ = y;
    tooltip_visible_ = true;
}

void ui_system::hide_tooltip()
{
    tooltip_visible_ = false;
}

void ui_system::set_focus(ui_element* element)
{
    if (focused_)
    {
        focused_->set_focused(false);
    }
    focused_ = element;
    if (focused_)
    {
        focused_->set_focused(true);
    }
}

void ui_system::clear_focus_if_owned_by(ui_element* root)
{
    if (!focused_ || !root)
    {
        return;
    }
    for (auto* el = focused_; el != nullptr; el = el->parent())
    {
        if (el == root)
        {
            focused_ = nullptr;
            return;
        }
    }
}

bool ui_system::is_modal_open() const
{
    // Check data-driven dialogs
    if (dialog_manager_ && dialog_manager_->is_modal_open())
    {
        return true;
    }

    // Check legacy dialogs
    for (const auto& [type, dlg] : dialogs_)
    {
        if (dlg->is_open() && dlg->modal())
        {
            return true;
        }
    }
    return false;
}

bool ui_system::has_text_focus() const
{
    if (focused_)
        return true;
    if (text_input_active_)
        return true;

    // Check chat dialog search
    if (auto it = dialogs_.find(dialog_type::chat); it != dialogs_.end())
    {
        if (auto* chat = dynamic_cast<const chat_dialog*>(it->second.get()))
        {
            if (chat->is_open() && chat->is_search_focused())
                return true;
        }
    }

    return false;
}

bool ui_system::is_point_over_dialog(int32_t x, int32_t y) const
{
    // Check data-driven dialogs
    if (dialog_manager_ && dialog_manager_->is_point_over_dialog(x, y))
    {
        return true;
    }

    // Check legacy dialogs
    for (const auto& [type, dlg] : dialogs_)
    {
        if (dlg->is_open() && dlg->bounds().contains(x, y))
        {
            return true;
        }
    }
    return false;
}

bool ui_system::is_mouse_consumed(sf::Mouse::Button btn) const
{
    if (btn == sf::Mouse::Button::Left)
        return mouse_consumed_left_;
    if (btn == sf::Mouse::Button::Right)
        return mouse_consumed_right_;
    return false;
}

void ui_system::update_mouse_consumed(const input& inp)
{
    // On press: mark consumed if over a dialog.
    // Don't overwrite if already set (update() may have consumed it before the dialog closed).
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left) && !mouse_consumed_left_)
    {
        mouse_consumed_left_ = is_point_over_dialog(inp.mouse_x(), inp.mouse_y());
    }
    if (inp.is_mouse_pressed(sf::Mouse::Button::Right) && !mouse_consumed_right_)
    {
        mouse_consumed_right_ = is_point_over_dialog(inp.mouse_x(), inp.mouse_y());
    }

    // On release: clear consumed flag
    if (inp.is_mouse_released(sf::Mouse::Button::Left))
    {
        mouse_consumed_left_ = false;
    }
    if (inp.is_mouse_released(sf::Mouse::Button::Right))
    {
        mouse_consumed_right_ = false;
    }
}

void ui_system::bring_to_front(dialog* dlg)
{
    auto it = std::find(dialog_order_.begin(), dialog_order_.end(), dlg);
    if (it != dialog_order_.end())
    {
        dialog_order_.erase(it);

        if (dlg->always_on_top())
        {
            // Always-on-top dialogs go at the very end
            dialog_order_.push_back(dlg);
        }
        else
        {
            // Insert before the first always-on-top dialog
            auto insert_pos =
                std::find_if(dialog_order_.begin(), dialog_order_.end(), [](dialog* d) { return d->always_on_top(); });
            dialog_order_.insert(insert_pos, dlg);
        }
    }
}

void ui_system::create_character_create_dialog()
{
    auto dlg = std::make_unique<character_create_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::character_create] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_icon_panel_dialog()
{
    // Try to use YAML-based icon panel if definition exists
    if (dialog_manager_)
    {
        auto* def = dialog_manager_->get_definition("icon_panel");
        if (def)
        {
            spdlog::info("Found YAML icon_panel definition, creating YAML-based dialog");
            yaml_icon_panel_ = dialog_manager_->create_icon_panel_dialog();
            if (yaml_icon_panel_)
            {
                spdlog::info("Using YAML-based icon panel (sprites_={})", yaml_icon_panel_->sprites() != nullptr);
                return;
            }
            else
            {
                spdlog::warn("Failed to create YAML-based icon panel");
            }
        }
        else
        {
            spdlog::info("No YAML icon_panel definition found, using code-based icon panel");
        }
    }
    else
    {
        spdlog::warn("dialog_manager_ is null when creating icon panel");
    }

    // Fallback to code-based icon panel
    spdlog::info("Creating code-based icon panel");
    auto dlg = std::make_unique<icon_panel_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::icon_panel] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_gauge_panel_dialog()
{
    auto dlg = std::make_unique<gauge_panel_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::gauge_panel] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_levelup_dialog()
{
    auto dlg = std::make_unique<levelup_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::levelup] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_fishing_dialog()
{
    auto dlg = std::make_unique<fishing_dialog>();
    dialog* ptr = dlg.get();
    dialogs_[dialog_type::fishing] = std::move(dlg);
    dialog_order_.push_back(ptr);
}

void ui_system::create_quest_dialog()
{
    add_dialog(dialog_type::quest, std::make_unique<quest_dialog>());
}

void ui_system::create_level_up_settings_dialog()
{
    add_dialog(dialog_type::level_up_settings, std::make_unique<level_up_settings_dialog>());
}

void ui_system::show_connection_dialog(std::function<void()> on_cancel)
{
    // Remove existing connection dialog if any
    hide_connection_dialog();

    auto dlg = std::make_unique<connection_dialog>();
    dlg->set_on_cancel(std::move(on_cancel));

    dialog* ptr = dlg.get();
    dialogs_[dialog_type::connection] = std::move(dlg);
    dialog_order_.push_back(ptr);
    static_cast<connection_dialog*>(ptr)->open();
}

void ui_system::show_error_dialog(std::string_view message, std::function<void()> on_cancel)
{
    // Remove existing connection dialog if any
    hide_connection_dialog();

    auto dlg = std::make_unique<connection_dialog>();
    dlg->set_error_mode(true, message);
    dlg->set_on_cancel(std::move(on_cancel));

    dialog* ptr = dlg.get();
    dialogs_[dialog_type::connection] = std::move(dlg);
    dialog_order_.push_back(ptr);
    static_cast<connection_dialog*>(ptr)->open();
}

void ui_system::hide_connection_dialog()
{
    // Close and remove connection dialog
    // IMPORTANT: Remove from dialog_order_ BEFORE erasing from dialogs_
    // to avoid use-after-free (dialogs_ owns the object via unique_ptr)
    auto it = dialogs_.find(dialog_type::connection);
    if (it != dialogs_.end())
    {
        dialog* ptr = it->second.get();
        clear_focus_if_owned_by(ptr);
        ptr->close();
        dialog_order_.erase(std::remove(dialog_order_.begin(), dialog_order_.end(), ptr), dialog_order_.end());
        dialogs_.erase(it);
    }
}

void ui_system::load_dialog_definitions(const std::filesystem::path& path)
{
    if (!dialog_manager_)
        return;

    // Set render mode based on current UI style
    render_mode mode = (style_ == ui_style::classic) ? render_mode::classic : render_mode::modern;
    dialog_manager_->set_default_render_mode(mode);

    dialog_manager_->load_definitions_from_yaml(path);
}

void ui_system::load_dialog_definitions_from_directory(const std::filesystem::path& dir)
{
    if (!dialog_manager_)
        return;

    // Set render mode based on current UI style
    render_mode mode = (style_ == ui_style::classic) ? render_mode::classic : render_mode::modern;
    dialog_manager_->set_default_render_mode(mode);

    dialog_manager_->load_definitions_from_directory(dir);
}

dialog_manager& ui_system::dialogs()
{
    return *dialog_manager_;
}

const dialog_manager& ui_system::dialogs() const
{
    return *dialog_manager_;
}

// === Item drag system ===

void ui_system::begin_item_drag(const item& itm, uint32_t item_id, equip_pos equip, dialog_type source,
                                int32_t cursor_x, int32_t cursor_y, int32_t offset_x, int32_t offset_y)
{
    drag_state_.active = true;
    drag_state_.held_item = itm;
    drag_state_.source_item_id = item_id;
    drag_state_.source_equip = equip;
    drag_state_.source_dialog = source;
    drag_state_.cursor_x = cursor_x;
    drag_state_.cursor_y = cursor_y;
    drag_state_.offset_x = offset_x;
    drag_state_.offset_y = offset_y;
}

void ui_system::update_drag_position(int32_t x, int32_t y)
{
    drag_state_.cursor_x = x;
    drag_state_.cursor_y = y;
}

void ui_system::cancel_item_drag()
{
    if (auto* inv_dlg = dynamic_cast<inventory_dialog*>(get_dialog(dialog_type::inventory)))
        inv_dlg->clear_dragging_item();
    drag_state_ = {};
}

void ui_system::end_item_drag(int32_t x, int32_t y, bool shift_held)
{
    if (!drag_state_.active)
        return;

    // Check if dropped on inventory dialog
    if (auto* inv_dlg = dynamic_cast<inventory_dialog*>(get_dialog(dialog_type::inventory));
        inv_dlg && inv_dlg->is_open() && inv_dlg->bounds().contains(x, y))
    {
        if (drag_state_.source_dialog == dialog_type::inventory)
        {
            // Item position was already updated by inventory_dialog::handle_mouse_move.
            // Just notify the reposition callback with the item's current bag-relative coords.
            if (on_reposition_item_)
            {
                auto bag = inv_dlg->bag_area();
                int32_t item_x = x - drag_state_.offset_x - bag.x;
                int32_t item_y = y - drag_state_.offset_y - bag.y;
                on_reposition_item_(drag_state_.source_item_id, item_x, item_y, shift_held);
            }
        }
        else if (drag_state_.source_dialog == dialog_type::character_info)
        {
            // Unequip
            if (on_unequip_from_drag_ && drag_state_.source_equip != equip_pos::none)
                on_unequip_from_drag_(drag_state_.source_equip);
        }
    }
    // Check if dropped on character info dialog (equip)
    else if (auto* chr = get_dialog(dialog_type::character_info);
             chr && chr->is_open() && chr->bounds().contains(x, y))
    {
        if (drag_state_.source_dialog == dialog_type::inventory && on_equip_from_drag_)
            on_equip_from_drag_(drag_state_.source_item_id);
    }
    // Dropped in game world
    else if (!is_point_over_dialog(x, y))
    {
        if (drag_state_.source_item_id != 0 && on_drop_in_world_)
            on_drop_in_world_(drag_state_.source_item_id);
    }

    if (auto* inv_dlg = dynamic_cast<inventory_dialog*>(get_dialog(dialog_type::inventory)))
        inv_dlg->clear_dragging_item();
    drag_state_ = {};
}

void ui_system::render_held_item_info(renderer& rend, const item& itm, int32_t x, int32_t y)
{
    auto lines = build_item_info(itm);
    if (lines.empty())
        return;

    int32_t line_height = 16;
    int32_t padding = 6;
    int32_t max_width = 0;
    for (const auto& line : lines)
        max_width = std::max(max_width, static_cast<int32_t>(line.text.length()) * 7);

    int32_t panel_w = max_width + padding * 2;
    int32_t panel_h = static_cast<int32_t>(lines.size()) * line_height + padding * 2;

    // Draw panel background
    rend.draw_rect(x, y, panel_w, panel_h, sf::Color(0, 0, 0, 180), true);
    rend.draw_rect(x, y, panel_w, panel_h, sf::Color(80, 80, 100), false);

    // Draw lines
    int32_t text_y = y + padding;
    for (const auto& line : lines)
    {
        rend.draw_text(line.text, x + padding, text_y, line.color);
        text_y += line_height;
    }
}

} // namespace hb
