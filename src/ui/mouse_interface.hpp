#pragma once

#include <cstdint>
#include <vector>

namespace hb
{

// Click detection result - matches original DEF_MIRESULT_*
enum class mouse_result : uint8_t
{
    none = 0,
    click = 1,
    drag = 2
};

// Click detection rectangle
struct click_rect
{
    int32_t x1, y1; // Top-left corner
    int32_t x2, y2; // Bottom-right corner
};

// Mouse interface for UI click detection
// Modern equivalent of CMouseInterface from the original code
class mouse_interface
{
public:
    mouse_interface() = default;
    ~mouse_interface() = default;

    mouse_interface(const mouse_interface&) = delete;
    mouse_interface& operator=(const mouse_interface&) = delete;
    mouse_interface(mouse_interface&&) noexcept = default;
    mouse_interface& operator=(mouse_interface&&) noexcept = default;

    // Add a clickable rectangle (returns the button number, 1-based)
    int32_t add_rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

    // Clear all rectangles
    void clear();

    // Check status - returns button number (1-based) and result through reference
    // mouse_pressed should be true if left mouse button was just pressed this frame
    int32_t get_status(int32_t mouse_x, int32_t mouse_y, bool mouse_pressed, mouse_result& result);

    // Check if point is in any rectangle (returns button number, 1-based, or 0 if none)
    int32_t hit_test(int32_t x, int32_t y) const;

private:
    std::vector<click_rect> rects_;
    bool was_pressed_ = false;
};

} // namespace hb
