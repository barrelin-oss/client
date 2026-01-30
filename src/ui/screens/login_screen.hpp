#pragma once

#include "ui/screens/screen_base.hpp"
#include <functional>
#include <string>

namespace hb {

// Sprite IDs for login screen
namespace login_sprites {
    inline constexpr uint16_t background = 53;  // DEF_SPRID_INTERFACE_ND_LOGIN
    inline constexpr uint16_t mouse_cursor = 0; // DEF_SPRID_MOUSECURSOR
    inline constexpr uint16_t game4 = 63;       // DEF_SPRID_INTERFACE_ND_GAME4 (for gateway message)
}

// Login button IDs
enum class login_focus : int32_t {
    account_name = 1,
    password = 2,
    connect = 3,
    cancel = 4
};

// Login screen - modern equivalent of UpdateScreen_OnLogin
class login_screen : public screen_base {
public:
    using login_callback = std::function<void(const std::string& name, const std::string& password)>;
    using cancel_callback = std::function<void()>;

    login_screen() = default;
    ~login_screen() override = default;

    void on_enter() override;
    void on_exit() override;
    bool update(float delta_time, const input& inp) override;
    void render(renderer& rend, sprite_manager& sprites) override;
    void render_cursor(renderer& rend, sprite_manager& sprites) override;

    // Set callbacks
    void set_on_login(login_callback callback) { on_login_ = std::move(callback); }
    void set_on_cancel(cancel_callback callback) { on_cancel_ = std::move(callback); }

    // Get current input values
    const std::string& account_name() const { return account_name_; }
    const std::string& password() const { return password_; }

private:
    void draw(renderer& rend, sprite_manager& sprites, int32_t mouse_x, int32_t mouse_y);
    void handle_text_input(const input& inp);
    void try_login();

    login_callback on_login_;
    cancel_callback on_cancel_;

    std::string account_name_;
    std::string password_;

    // Text input settings (matching original)
    static constexpr int32_t account_max_length = 11;
    static constexpr int32_t password_max_length = 11;

    // UI positions (matching original)
    static constexpr int32_t text_input_x = 180;
    static constexpr int32_t account_y = 162;
    static constexpr int32_t password_y = 185;

    // Cursor blink state
    float cursor_timer_ = 0.0f;
    bool cursor_visible_ = true;
};

} // namespace hb
