#pragma once

#include "ui/screens/screen_base.hpp"
#include <functional>
#include <string>

#ifdef HB_DEBUG_OVERLAY_ENABLED
#include "debug/screen_adapters.hpp"
#include <memory>
#include <vector>
#endif

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

    using settings_callback = std::function<void()>;

    // Set callbacks
    void set_on_login(login_callback callback) { on_login_ = std::move(callback); }
    void set_on_cancel(cancel_callback callback) { on_cancel_ = std::move(callback); }
    void set_on_settings(settings_callback callback) { on_settings_ = std::move(callback); }

    // Get current input values
    const std::string& account_name() const { return account_name_; }
    const std::string& password() const { return password_; }

private:
    void draw(renderer& rend, sprite_manager& sprites);
    void handle_text_input(const input& inp);
    void try_login();

    login_callback on_login_;
    cancel_callback on_cancel_;
    settings_callback on_settings_;

    std::string account_name_;
    std::string password_;

    // Text input settings (matching original)
    static constexpr int32_t account_max_length = 11;
    static constexpr int32_t password_max_length = 11;

    // Cursor blink state
    float cursor_timer_ = 0.0f;
    bool cursor_visible_ = true;

    // Settings button (bottom-right corner, resolution-independent)
    static constexpr int32_t settings_btn_width_ = 80;
    static constexpr int32_t settings_btn_height_ = 28;
    static constexpr int32_t settings_btn_margin_ = 12;
    bool settings_hovered_ = false;
    uint32_t window_width_ = 0;
    uint32_t window_height_ = 0;

    // UI positions (mutable for debug overlay positioning)
    // Login panel position
    int32_t panel_x_ = 39;
    int32_t panel_y_ = 122;
    static constexpr int32_t panel_width_ = 332;
    static constexpr int32_t panel_height_ = 184;

    // Input box positions
    int32_t account_box_x_ = 171;
    int32_t account_box_y_ = 156;
    int32_t password_box_x_ = 171;
    int32_t password_box_y_ = 180;
    static constexpr int32_t input_box_width_ = 197;
    static constexpr int32_t input_box_height_ = 20;

#ifdef HB_DEBUG_OVERLAY_ENABLED
    // Debug overlay adapters
    void register_debug_adapters();
    void unregister_debug_adapters();
    std::vector<std::unique_ptr<debug::screen_point_adapter>> debug_adapters_;
#endif
};

} // namespace hb
