#include "graphics/screen_transition.hpp"
#include "graphics/renderer.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace hb
{

// ── helpers ────────────────────────────────────────────────────────────

static float ease_out_cubic(float t)
{
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static float ease_in_cubic(float t)
{
    return t * t * t;
}

// ── transition_type_name ───────────────────────────────────────────────

std::string_view transition_type_name(transition_type type)
{
    switch (type)
    {
    case transition_type::diamond_wave:
        return "Diamond Wave";
    case transition_type::circle_expand:
        return "Circle Expand";
    case transition_type::horizontal_blinds:
        return "Horizontal Blinds";
    case transition_type::vertical_blinds:
        return "Vertical Blinds";
    case transition_type::diagonal_wipe:
        return "Diagonal Wipe";
    case transition_type::rain:
        return "Rain";
    default:
        return "Unknown";
    }
}

// ── next_type ──────────────────────────────────────────────────────────

transition_type screen_transition::next_type()
{
    auto idx = static_cast<uint8_t>(type_);
    idx = (idx + 1) % transition_type_count;
    type_ = static_cast<transition_type>(idx);
    return type_;
}

// ── randomize_type ─────────────────────────────────────────────────────

transition_type screen_transition::randomize_type()
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, transition_type_count - 1);
    type_ = static_cast<transition_type>(dist(rng));
    return type_;
}

// ── cancel ─────────────────────────────────────────────────────────────

void screen_transition::cancel()
{
    phase_ = transition_phase::idle;
    cells_.clear();
}

// ── build_grid ─────────────────────────────────────────────────────────

void screen_transition::build_grid(uint32_t screen_width, uint32_t screen_height)
{
    cells_.clear();
    screen_width_ = screen_width;
    screen_height_ = screen_height;

    grid_cols_ = static_cast<int32_t>((screen_width + target_cell_size - 1) / target_cell_size);
    grid_rows_ = static_cast<int32_t>((screen_height + target_cell_size - 1) / target_cell_size);
    if (grid_cols_ < 1)
        grid_cols_ = 1;
    if (grid_rows_ < 1)
        grid_rows_ = 1;

    float cell_w = static_cast<float>(screen_width) / static_cast<float>(grid_cols_);
    float cell_h = static_cast<float>(screen_height) / static_cast<float>(grid_rows_);

    cells_.reserve(static_cast<size_t>(grid_cols_ * grid_rows_));

    for (int32_t r = 0; r < grid_rows_; ++r)
    {
        for (int32_t c = 0; c < grid_cols_; ++c)
        {
            cell cl{};
            cl.center_x = c * cell_w + cell_w * 0.5f;
            cl.center_y = r * cell_h + cell_h * 0.5f;
            cl.half_w = cell_w * 0.5f;
            cl.half_h = cell_h * 0.5f;
            cl.normalized_delay = 0.0f;
            cl.slide_dx = 0.0f;
            cl.slide_dy = 0.0f;
            cells_.push_back(cl);
        }
    }

    assign_delays();
}

// ── assign_delays ──────────────────────────────────────────────────────
// Sets normalized_delay [0,1], slide direction, animation style, and
// timing constants based on the current transition_type.

void screen_transition::assign_delays()
{
    float center_col = (grid_cols_ - 1) * 0.5f;
    float center_row = (grid_rows_ - 1) * 0.5f;

    // Default timing
    open_stagger_ = 0.7f;
    open_cell_time_ = 0.35f;
    close_stagger_ = 0.35f;
    close_cell_time_ = 0.2f;
    anim_style_ = cell_anim::shrink;

    switch (type_)
    {
    // ── Diamond Wave ───────────────────────────────────────────────
    case transition_type::diamond_wave:
    {
        float max_dist = center_col + center_row;
        if (max_dist < 1.0f)
            max_dist = 1.0f;

        for (int32_t r = 0; r < grid_rows_; ++r)
        {
            for (int32_t c = 0; c < grid_cols_; ++c)
            {
                auto& cl = cells_[static_cast<size_t>(r * grid_cols_ + c)];
                float dist =
                    std::abs(static_cast<float>(c) - center_col) + std::abs(static_cast<float>(r) - center_row);
                cl.normalized_delay = dist / max_dist;
            }
        }
        open_stagger_ = 0.7f;
        open_cell_time_ = 0.35f;
        break;
    }

    // ── Circle Expand ──────────────────────────────────────────────
    case transition_type::circle_expand:
    {
        float max_dist = std::sqrt(center_col * center_col + center_row * center_row);
        if (max_dist < 1.0f)
            max_dist = 1.0f;

        for (int32_t r = 0; r < grid_rows_; ++r)
        {
            for (int32_t c = 0; c < grid_cols_; ++c)
            {
                auto& cl = cells_[static_cast<size_t>(r * grid_cols_ + c)];
                float dx = static_cast<float>(c) - center_col;
                float dy = static_cast<float>(r) - center_row;
                cl.normalized_delay = std::sqrt(dx * dx + dy * dy) / max_dist;
            }
        }
        open_stagger_ = 0.75f;
        open_cell_time_ = 0.3f;
        break;
    }

    // ── Horizontal Blinds ──────────────────────────────────────────
    case transition_type::horizontal_blinds:
    {
        anim_style_ = cell_anim::slide;

        for (int32_t r = 0; r < grid_rows_; ++r)
        {
            // Stagger from center rows outward
            float row_delay = std::abs(static_cast<float>(r) - center_row) / std::max(center_row, 1.0f);

            for (int32_t c = 0; c < grid_cols_; ++c)
            {
                auto& cl = cells_[static_cast<size_t>(r * grid_cols_ + c)];
                cl.normalized_delay = row_delay;
                // Alternate rows slide up vs down
                cl.slide_dx = 0.0f;
                cl.slide_dy = (r % 2 == 0) ? -1.0f : 1.0f;
            }
        }
        open_stagger_ = 0.5f;
        open_cell_time_ = 0.4f;
        break;
    }

    // ── Vertical Blinds ───────────────────────────────────────────
    case transition_type::vertical_blinds:
    {
        anim_style_ = cell_anim::slide;

        for (int32_t r = 0; r < grid_rows_; ++r)
        {
            for (int32_t c = 0; c < grid_cols_; ++c)
            {
                auto& cl = cells_[static_cast<size_t>(r * grid_cols_ + c)];
                // Stagger from center columns outward
                cl.normalized_delay = std::abs(static_cast<float>(c) - center_col) / std::max(center_col, 1.0f);
                // Alternate columns slide left vs right
                cl.slide_dx = (c % 2 == 0) ? -1.0f : 1.0f;
                cl.slide_dy = 0.0f;
            }
        }
        open_stagger_ = 0.5f;
        open_cell_time_ = 0.4f;
        break;
    }

    // ── Diagonal Wipe ──────────────────────────────────────────────
    case transition_type::diagonal_wipe:
    {
        anim_style_ = cell_anim::slide;
        float max_sum = static_cast<float>(grid_cols_ - 1 + grid_rows_ - 1);
        if (max_sum < 1.0f)
            max_sum = 1.0f;

        for (int32_t r = 0; r < grid_rows_; ++r)
        {
            for (int32_t c = 0; c < grid_cols_; ++c)
            {
                auto& cl = cells_[static_cast<size_t>(r * grid_cols_ + c)];
                cl.normalized_delay = static_cast<float>(c + r) / max_sum;
                // Slide diagonally toward top-left
                cl.slide_dx = -0.7f;
                cl.slide_dy = -0.7f;
            }
        }
        open_stagger_ = 0.7f;
        open_cell_time_ = 0.35f;
        break;
    }

    // ── Rain ───────────────────────────────────────────────────────
    case transition_type::rain:
    {
        anim_style_ = cell_anim::slide;

        // Deterministic per-column offset for organic feel
        std::mt19937 rng(123);
        std::uniform_real_distribution<float> jitter(0.0f, 0.15f);

        std::vector<float> col_offsets(static_cast<size_t>(grid_cols_));
        for (auto& off : col_offsets)
            off = jitter(rng);

        float max_row = std::max(static_cast<float>(grid_rows_ - 1), 1.0f);

        for (int32_t r = 0; r < grid_rows_; ++r)
        {
            for (int32_t c = 0; c < grid_cols_; ++c)
            {
                auto& cl = cells_[static_cast<size_t>(r * grid_cols_ + c)];
                float base = static_cast<float>(r) / max_row;
                cl.normalized_delay = std::clamp(base + col_offsets[static_cast<size_t>(c)], 0.0f, 1.0f);
                // All cells slide downward
                cl.slide_dx = 0.0f;
                cl.slide_dy = 1.0f;
            }
        }
        open_stagger_ = 0.6f;
        open_cell_time_ = 0.35f;
        break;
    }

    default:
        break;
    }
}

// ── start_reveal / start_full ──────────────────────────────────────────

void screen_transition::start_reveal(uint32_t screen_width, uint32_t screen_height)
{
    build_grid(screen_width, screen_height);
    phase_ = transition_phase::opening;
    elapsed_ = 0.0f;
    on_midpoint_ = nullptr;
    midpoint_fired_ = false;
}

void screen_transition::start_full(uint32_t screen_width, uint32_t screen_height, std::function<void()> on_midpoint)
{
    build_grid(screen_width, screen_height);
    phase_ = transition_phase::closing;
    elapsed_ = 0.0f;
    on_midpoint_ = std::move(on_midpoint);
    midpoint_fired_ = false;
}

// ── update ─────────────────────────────────────────────────────────────

void screen_transition::update(float delta_time)
{
    if (phase_ == transition_phase::idle)
        return;

    elapsed_ += delta_time;

    if (phase_ == transition_phase::closing)
    {
        float total = close_stagger_ + close_cell_time_;
        if (elapsed_ >= total)
        {
            if (on_midpoint_ && !midpoint_fired_)
            {
                midpoint_fired_ = true;
                on_midpoint_();
            }
            phase_ = transition_phase::opening;
            elapsed_ = 0.0f;
        }
    }
    else if (phase_ == transition_phase::opening)
    {
        float total = open_stagger_ + open_cell_time_;
        if (elapsed_ >= total)
        {
            phase_ = transition_phase::idle;
            cells_.clear();
        }
    }
}

// ── render ─────────────────────────────────────────────────────────────

void screen_transition::render(renderer& rend)
{
    if (phase_ == transition_phase::idle)
        return;

    float cell_pixel_size = static_cast<float>(target_cell_size);

    for (const auto& cl : cells_)
    {
        float t;

        if (phase_ == transition_phase::opening)
        {
            float start_time = cl.normalized_delay * open_stagger_;
            t = std::clamp((elapsed_ - start_time) / open_cell_time_, 0.0f, 1.0f);
        }
        else // closing
        {
            // Invert delay for closing: edges first (1-delay), center last
            float start_time = (1.0f - cl.normalized_delay) * close_stagger_;
            t = std::clamp((elapsed_ - start_time) / close_cell_time_, 0.0f, 1.0f);
        }

        // ── compute alpha, scale, and offset per animation style ───
        float alpha;
        float scale;
        float offset_x = 0.0f;
        float offset_y = 0.0f;

        bool is_opening = (phase_ == transition_phase::opening);

        switch (anim_style_)
        {
        case cell_anim::shrink:
        {
            float eased = is_opening ? ease_out_cubic(t) : ease_in_cubic(t);
            if (is_opening)
            {
                alpha = 1.0f - eased;
                scale = 1.0f - eased * 0.55f;
            }
            else
            {
                alpha = eased;
                scale = 0.45f + eased * 0.55f;
            }
            break;
        }

        case cell_anim::slide:
        {
            float eased = is_opening ? ease_out_cubic(t) : ease_in_cubic(t);
            if (is_opening)
            {
                alpha = 1.0f - eased;
                scale = 1.0f;
                offset_x = cl.slide_dx * eased * cell_pixel_size;
                offset_y = cl.slide_dy * eased * cell_pixel_size;
            }
            else
            {
                alpha = eased;
                scale = 1.0f;
                offset_x = cl.slide_dx * (1.0f - eased) * cell_pixel_size;
                offset_y = cl.slide_dy * (1.0f - eased) * cell_pixel_size;
            }
            break;
        }

        default:
            alpha = is_opening ? (1.0f - t) : t;
            scale = 1.0f;
            break;
        }

        if (alpha < 0.004f)
            continue;

        uint8_t a = static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
        float w = cl.half_w * 2.0f * scale;
        float h = cl.half_h * 2.0f * scale;
        float x = cl.center_x - w * 0.5f + offset_x;
        float y = cl.center_y - h * 0.5f + offset_y;

        rend.draw_rect(static_cast<int32_t>(x),
                       static_cast<int32_t>(y),
                       static_cast<int32_t>(std::ceil(w)),
                       static_cast<int32_t>(std::ceil(h)),
                       sf::Color(overlay_r, overlay_g, overlay_b, a),
                       true);
    }

    // ── label ──────────────────────────────────────────────────────
    if (show_label_)
    {
        auto name = transition_type_name(type_);
        // Draw in bottom-left with outline for readability
        int32_t lx = 12;
        int32_t ly = static_cast<int32_t>(screen_height_) - 36;
        rend.draw_text_outlined(name, lx, ly, sf::Color(220, 200, 140), sf::Color::Black, 16, 2.0f);
    }
}

} // namespace hb
