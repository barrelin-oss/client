#pragma once

#include "graphics/color_utils.hpp"
#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace hb {

class renderer;

// Severity levels for keyed status messages (legacy API)
enum class status_severity
{
    info,      // White/gray text
    warning,   // Yellow text
    critical   // Red text, flashing
};

// A keyed status message (persistent, updateable)
struct status_message
{
    std::string key;           // Unique identifier for updating/removing
    std::string text;          // Display text
    status_severity severity = status_severity::info;
    float lifetime = -1.0f;    // Seconds until auto-remove (-1 = persistent)
    float elapsed = 0.0f;      // Time since message was added/updated
};

// A fire-and-forget event message (like legacy AddEventList)
struct event_message
{
    std::string text;
    message_color color = message_color::white;
    float lifetime = 5.0f;     // 5 seconds like legacy
    float elapsed = 0.0f;
};

// Status log for displaying messages in bottom-left corner
// Handles both:
// 1. Keyed status messages (persistent, updateable) - e.g., hunger status
// 2. Event notifications (fire-and-forget, scrolling) - e.g., combat messages
class status_log
{
public:
    status_log() = default;
    ~status_log() = default;

    // Update timers and animations
    void update(float delta_time);

    // Render all active messages
    // icon_panel_height: height of the icon panel to position above
    void render(renderer& rend, int32_t screen_width, int32_t screen_height,
                int32_t icon_panel_height = 70);

    // --- Keyed status messages (persistent, updateable) ---

    // Add or update a message by key
    // If a message with this key exists, it will be updated
    // lifetime: seconds until auto-remove, -1 for persistent
    void set_message(const std::string& key, const std::string& text,
                     status_severity severity, float lifetime = -1.0f);

    // Remove a message by key
    void remove_message(const std::string& key);

    // Check if a message exists
    bool has_message(const std::string& key) const;

    // --- Event notifications (fire-and-forget, scrolling) ---

    // Add an event message (like legacy AddEventList)
    // allow_duplicates: if false, won't add if last event has same text
    void add_event(std::string_view text,
                   message_color color = message_color::white,
                   bool allow_duplicates = true);

    // --- General ---

    // Clear all messages and events
    void clear();

    // Clear only events (keep keyed messages)
    void clear_events();

private:
    // Keyed status messages
    std::vector<status_message> messages_;

    // Event messages (rolling buffer, newest at back)
    std::deque<event_message> events_;

    // Animation state
    float flash_timer_ = 0.0f;     // For critical message flashing
    float rainbow_time_ = 0.0f;   // For rainbow color animation

    // Constants
    static constexpr size_t max_events = 6;
    static constexpr float event_lifetime = 5.0f;
    static constexpr float fade_duration = 0.5f;
};

} // namespace hb
