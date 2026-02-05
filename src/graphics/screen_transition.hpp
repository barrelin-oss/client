#pragma once

#include <SFML/Graphics/Color.hpp>
#include <cstdint>
#include <string_view>
#include <vector>
#include <functional>

namespace hb {

class renderer;

enum class transition_type : uint8_t
{
    diamond_wave,       // Manhattan distance from center - classic JRPG iris
    circle_expand,      // Euclidean distance from center - smooth radial
    horizontal_blinds,  // Horizontal strips retract alternating up/down
    vertical_blinds,    // Vertical strips retract alternating left/right
    random_scatter,     // Random delays for a dissolve effect
    spiral,             // Cells follow a spiral pattern from center
    diagonal_wipe,      // Diagonal line sweeps corner to corner
    rain,               // Cells fall away top-to-bottom in staggered columns
    checkerboard,       // Even/odd cells in two quick phases
    vortex,             // Swirling whirlpool pattern

    count               // Number of transition types
};

inline constexpr auto transition_type_count = static_cast<uint8_t>(transition_type::count);

std::string_view transition_type_name(transition_type type);

enum class transition_phase : uint8_t
{
    idle,
    closing,    // Scene being covered
    opening     // Scene being revealed
};

// Per-cell animation style (set by transition type)
enum class cell_anim : uint8_t
{
    shrink,         // Shrink toward center while fading
    expand_pop,     // Expand outward while fading (pop/burst)
    slide,          // Slide in a direction while fading
};

class screen_transition
{
public:
    screen_transition() = default;

    // Reveal from black (opening only). Used when entering game.
    void start_reveal(uint32_t screen_width, uint32_t screen_height);

    // Full close-then-open. The midpoint callback fires when the screen
    // is fully covered - use it to load maps, reposition the player, etc.
    void start_full(uint32_t screen_width, uint32_t screen_height,
                    std::function<void()> on_midpoint = nullptr);

    void update(float delta_time);
    void render(renderer& rend);

    bool is_active() const { return phase_ != transition_phase::idle; }

    // True during close phase - gameplay input should be blocked.
    bool is_blocking() const { return phase_ == transition_phase::closing; }

    // Current transition type
    transition_type type() const { return type_; }
    void set_type(transition_type type) { type_ = type; }

    // Cycle to next type and return it
    transition_type next_type();

    // Pick a random transition type
    transition_type randomize_type();

    // Immediately end any active transition
    void cancel();

    // Enable/disable name label rendering during transition
    void set_show_label(bool show) { show_label_ = show; }

private:
    struct cell
    {
        float center_x;
        float center_y;
        float half_w;
        float half_h;
        float normalized_delay;   // 0.0 = first to animate, 1.0 = last
        float slide_dx;           // Slide direction X (-1, 0, 1)
        float slide_dy;           // Slide direction Y (-1, 0, 1)
    };

    void build_grid(uint32_t screen_width, uint32_t screen_height);
    void assign_delays();

    transition_type type_ = transition_type::diamond_wave;
    cell_anim anim_style_ = cell_anim::shrink;
    transition_phase phase_ = transition_phase::idle;
    float elapsed_ = 0.0f;
    bool show_label_ = false;

    // Timing per-type (set in assign_delays)
    float open_stagger_ = 0.7f;
    float open_cell_time_ = 0.35f;
    float close_stagger_ = 0.35f;
    float close_cell_time_ = 0.2f;

    int32_t grid_cols_ = 0;
    int32_t grid_rows_ = 0;

    std::vector<cell> cells_;
    std::function<void()> on_midpoint_;
    bool midpoint_fired_ = false;

    uint32_t screen_width_ = 0;
    uint32_t screen_height_ = 0;

    static constexpr int32_t target_cell_size = 45;

    // Overlay color: near-black with slight blue tint
    static constexpr uint8_t overlay_r = 5;
    static constexpr uint8_t overlay_g = 5;
    static constexpr uint8_t overlay_b = 15;
};

} // namespace hb
