#include "input/input.hpp"

namespace hb
{

void input::process_event(const sf::Event& event)
{
    // SFML 3.0 uses visitor pattern for events
    if (event.is<sf::Event::Closed>())
    {
        should_close_ = true;
    }
    else if (event.is<sf::Event::FocusGained>())
    {
        has_focus_ = true;
    }
    else if (event.is<sf::Event::FocusLost>())
    {
        has_focus_ = false;
    }
    else if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
    {
        mouse_x_ = moved->position.x;
        mouse_y_ = moved->position.y;
    }
    else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        auto btn = static_cast<size_t>(pressed->button);
        if (btn < max_mouse_buttons)
        {
            mouse_down_[btn] = true;
            mouse_pressed_[btn] = true;
        }
    }
    else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
    {
        auto btn = static_cast<size_t>(released->button);
        if (btn < max_mouse_buttons)
        {
            mouse_down_[btn] = false;
            mouse_released_[btn] = true;
        }
    }
    else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        if (wheel->wheel == sf::Mouse::Wheel::Vertical)
        {
            wheel_delta_ += static_cast<int32_t>(wheel->delta);
        }
    }
    else if (const auto* key_pressed = event.getIf<sf::Event::KeyPressed>())
    {
        auto key = static_cast<size_t>(key_pressed->code);
        if (key < max_keys)
        {
            key_down_[key] = true;
            key_pressed_[key] = true;
        }
        auto scan = static_cast<size_t>(key_pressed->scancode);
        if (scan < max_scancodes)
        {
            scan_down_[scan] = true;
            scan_pressed_[scan] = true;
        }
    }
    else if (const auto* key_released = event.getIf<sf::Event::KeyReleased>())
    {
        auto key = static_cast<size_t>(key_released->code);
        if (key < max_keys)
        {
            key_down_[key] = false;
            key_released_[key] = true;
        }
        auto scan = static_cast<size_t>(key_released->scancode);
        if (scan < max_scancodes)
        {
            scan_down_[scan] = false;
        }
    }
    else if (const auto* text = event.getIf<sf::Event::TextEntered>())
    {
        // Filter control characters, allow printable ASCII and Unicode
        if (text->unicode >= 32 && text->unicode != 127)
        {
            // Convert UTF-32 to UTF-8
            if (text->unicode < 0x80)
            {
                text_input_ += static_cast<char>(text->unicode);
            }
            else if (text->unicode < 0x800)
            {
                text_input_ += static_cast<char>(0xC0 | (text->unicode >> 6));
                text_input_ += static_cast<char>(0x80 | (text->unicode & 0x3F));
            }
            else if (text->unicode < 0x10000)
            {
                text_input_ += static_cast<char>(0xE0 | (text->unicode >> 12));
                text_input_ += static_cast<char>(0x80 | ((text->unicode >> 6) & 0x3F));
                text_input_ += static_cast<char>(0x80 | (text->unicode & 0x3F));
            }
        }
    }
}

void input::end_frame()
{
    // Reset per-frame state
    mouse_pressed_.fill(false);
    mouse_released_.fill(false);
    key_pressed_.fill(false);
    key_released_.fill(false);
    scan_pressed_.fill(false);
    wheel_delta_ = 0;
    text_input_.clear();
}

bool input::is_mouse_down(sf::Mouse::Button btn) const
{
    auto idx = static_cast<size_t>(btn);
    if (idx < max_mouse_buttons)
    {
        return mouse_down_[idx];
    }
    return false;
}

bool input::is_mouse_pressed(sf::Mouse::Button btn) const
{
    auto idx = static_cast<size_t>(btn);
    if (idx < max_mouse_buttons)
    {
        return mouse_pressed_[idx];
    }
    return false;
}

bool input::is_mouse_released(sf::Mouse::Button btn) const
{
    auto idx = static_cast<size_t>(btn);
    if (idx < max_mouse_buttons)
    {
        return mouse_released_[idx];
    }
    return false;
}

bool input::is_key_down(sf::Keyboard::Key key) const
{
    auto idx = static_cast<size_t>(key);
    if (idx < max_keys)
    {
        return key_down_[idx];
    }
    return false;
}

bool input::is_key_pressed(sf::Keyboard::Key key) const
{
    auto idx = static_cast<size_t>(key);
    if (idx < max_keys)
    {
        return key_pressed_[idx];
    }
    return false;
}

bool input::is_key_released(sf::Keyboard::Key key) const
{
    auto idx = static_cast<size_t>(key);
    if (idx < max_keys)
    {
        return key_released_[idx];
    }
    return false;
}

bool input::is_scan_down(sf::Keyboard::Scancode scan) const
{
    auto idx = static_cast<size_t>(scan);
    if (idx < max_scancodes)
    {
        return scan_down_[idx];
    }
    return false;
}

bool input::is_scan_pressed(sf::Keyboard::Scancode scan) const
{
    auto idx = static_cast<size_t>(scan);
    if (idx < max_scancodes)
    {
        return scan_pressed_[idx];
    }
    return false;
}

} // namespace hb
