#pragma once

#include "ui/screens/screen_base.hpp"
#include <functional>

namespace hb {

// Screen shown when server closes connection
// Displays "Connection lost" for 5 seconds, then returns to main menu
class connection_lost_screen : public screen_base
{
public:
    using timeout_callback = std::function<void()>;

    connection_lost_screen() = default;
    ~connection_lost_screen() override = default;

    void on_enter() override;
    void on_exit() override;
    bool update(float delta_time, const input& inp) override;
    void render(renderer& rend, sprite_manager& sprites) override;
    void render_cursor(renderer& rend, sprite_manager& sprites) override;

    // Set callback for when timeout expires (returns to main menu)
    void set_on_timeout(timeout_callback callback) { on_timeout_ = std::move(callback); }

    // Optional: set a reason message
    void set_reason(std::string_view reason) { reason_ = reason; }

private:
    static constexpr float timeout_duration_ = 5.0f;

    timeout_callback on_timeout_;
    std::string reason_;
    float elapsed_time_ = 0.0f;
};

} // namespace hb
