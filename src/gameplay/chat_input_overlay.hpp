#pragma once

#include "chat/chat_message.hpp"
#include <functional>
#include <string>
#include <string_view>

namespace hb {

class input;
class renderer;

// Lightweight always-available chat input overlay (not a dialog).
// Activated by Enter key, supports legacy Helbreath prefix modes.
class chat_input_overlay
{
public:
    // Callback: (content, channel, whisper_target)
    using send_callback = std::function<void(std::string_view, std::string_view, std::string_view)>;
    // Callback for server commands: (raw_command_text)
    using command_callback = std::function<void(std::string_view)>;

    void set_on_send(send_callback cb) { on_send_ = std::move(cb); }
    void set_on_command(command_callback cb) { on_command_ = std::move(cb); }
    void set_type_to_chat(bool enabled) { type_to_chat_ = enabled; }

    // Returns true when active (consuming keyboard input).
    bool update(float delta_time, const input& inp);
    void render(renderer& rend, int32_t screen_width, int32_t screen_height);

    bool is_active() const { return active_; }
    void activate();
    void deactivate();

    // Whisper state (for display in other UI)
    bool is_whisper_active() const { return whisper_active_; }
    std::string_view whisper_target() const { return whisper_target_; }

private:
    enum class chat_mode : uint8_t
    {
        say,       // local
        shout,     // !
        faction,   // ~
        guild,     // @
        party,     // $
        trade,     // %
        gm,        // ^
        whisper,   // /to
    };

    void send_current();
    void parse_command(std::string_view text);
    std::string_view channel_for_mode() const;
    sf::Color color_for_mode() const;
    std::string label_for_mode() const;

    send_callback on_send_;
    command_callback on_command_;

    bool active_ = false;
    std::string input_text_;
    size_t cursor_pos_ = 0;

    chat_mode mode_ = chat_mode::say;

    // Whisper persistence
    bool whisper_active_ = false;
    std::string whisper_target_;

    // Type-to-chat: any printable key opens chat (legacy behavior)
    bool type_to_chat_ = false;

    // Cursor blink
    float blink_timer_ = 0.0f;
    bool cursor_visible_ = true;

    static constexpr size_t max_input_length = 200;
};

} // namespace hb
