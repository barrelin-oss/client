#pragma once

#include "ui/screens/screen_base.hpp"
#include "ui/screens/main_menu_screen.hpp"
#include "ui/screens/login_screen.hpp"
#include "ui/screens/character_select_screen.hpp"
#include "ui/screens/character_create_screen.hpp"
#include "ui/screens/connection_lost_screen.hpp"
#include "ui/screens/reconnect_screen.hpp"
#include <memory>
#include <functional>

namespace hb
{

class renderer;
class input;
class sprite_manager;

// Screen types
enum class screen_type
{
    none,
    main_menu,
    login,
    character_select,
    character_create,
    loading,
    playing,
    connection_lost,
    reconnect
};

// Manages screen transitions and updates
class screen_manager
{
public:
    using state_change_callback = std::function<void(screen_type)>;

    screen_manager() = default;
    ~screen_manager() = default;

    screen_manager(const screen_manager&) = delete;
    screen_manager& operator=(const screen_manager&) = delete;

    // Initialize screens
    void initialize();
    void shutdown();

    // Screen transitions
    void change_screen(screen_type type);
    screen_type current_screen_type() const { return current_type_; }

    // Update current screen (input handling)
    void update(float delta_time, const input& inp);

    // Update just mouse position (for cursor rendering when input blocked)
    void update_mouse_position(const input& inp);

    // Render current screen (sprite drawing)
    void render(renderer& rend, sprite_manager& sprites);

    // Get specific screens for setup
    main_menu_screen& get_main_menu_screen() { return main_menu_; }
    login_screen& get_login_screen() { return login_; }
    character_select_screen& get_character_select_screen() { return character_select_; }
    character_create_screen& get_character_create_screen() { return character_create_; }
    connection_lost_screen& get_connection_lost_screen() { return connection_lost_; }
    reconnect_screen& get_reconnect_screen() { return reconnect_; }

    // Callbacks for state changes (so game_state can react)
    void set_on_state_change(state_change_callback callback) { on_state_change_ = std::move(callback); }

private:
    screen_base* get_screen(screen_type type);

    main_menu_screen main_menu_;
    login_screen login_;
    character_select_screen character_select_;
    character_create_screen character_create_;
    connection_lost_screen connection_lost_;
    reconnect_screen reconnect_;

    screen_type current_type_ = screen_type::none;
    screen_base* current_screen_ = nullptr;

    state_change_callback on_state_change_;
};

} // namespace hb
