#include "ui/screens/reconnect_screen.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "assets/sprite_manager.hpp"
#include <spdlog/spdlog.h>
#include <array>
#include <cmath>
#include <cstring>

namespace hb {

void reconnect_screen::on_enter()
{
    elapsed_time_ = 0.0f;
    status_text_.clear();
    spdlog::info("Reconnect screen entered (auto_connect={})", auto_connect_);
}

void reconnect_screen::on_exit()
{
    spdlog::info("Reconnect screen exited");
}

bool reconnect_screen::update(float delta_time, const input& inp)
{
    elapsed_time_ += delta_time;

    mouse_x_ = inp.mouse_x();
    mouse_y_ = inp.mouse_y();

    // Auto-connect on first show (fires once)
    if (auto_connect_)
    {
        auto_connect_ = false;
        if (on_reconnect_)
        {
            on_reconnect_();
        }
        return true;
    }

    // Enter to reconnect
    if (inp.is_key_pressed(sf::Keyboard::Key::Enter))
    {
        play_button_sound();
        if (on_reconnect_)
        {
            on_reconnect_();
        }
        return true;
    }

    // Check button click
    if (inp.is_mouse_pressed(sf::Mouse::Button::Left) && screen_width_ > 0)
    {
        int32_t btn_x = (screen_width_ - button_width_) / 2;
        int32_t btn_y = screen_height_ * 55 / 100;

        if (mouse_x_ >= btn_x && mouse_x_ <= btn_x + button_width_
            && mouse_y_ >= btn_y && mouse_y_ <= btn_y + button_height_)
        {
            play_button_sound();
            if (on_reconnect_)
            {
                on_reconnect_();
            }
            return true;
        }
    }

    return true;
}

void reconnect_screen::render(renderer& rend, sprite_manager& sprites)
{
    (void)sprites;

    screen_width_ = static_cast<int32_t>(rend.width());
    screen_height_ = static_cast<int32_t>(rend.height());

    auto sw = screen_width_;
    auto sh = screen_height_;

    // Dark navy background
    rend.draw_rect(0, 0, sw, sh, sf::Color(8, 8, 18));

    // Twinkling stars
    struct star { float x; float y; float offset; float size; };
    static const std::array<star, 40> stars = []() {
        std::array<star, 40> s{};
        for (size_t i = 0; i < 40; ++i)
        {
            uint32_t h = static_cast<uint32_t>(i) * 2654435761u;
            s[i].x = static_cast<float>(h % 1000u) / 1000.0f;
            h = h * 2246822519u + 3266489917u;
            s[i].y = static_cast<float>(h % 1000u) / 1000.0f;
            h = h * 374761393u;
            s[i].offset = static_cast<float>(h % 628u) / 100.0f;
            s[i].size = (h % 2u == 0) ? 1.0f : 2.0f;
        }
        return s;
    }();

    for (const auto& s : stars)
    {
        float alpha_f = 80.0f + 60.0f * std::sin(elapsed_time_ * 1.8f + s.offset);
        auto alpha = static_cast<uint8_t>(std::clamp(alpha_f, 0.0f, 255.0f));
        int32_t sx = static_cast<int32_t>(s.x * static_cast<float>(sw));
        int32_t sy = static_cast<int32_t>(s.y * static_cast<float>(sh));
        int32_t sz = static_cast<int32_t>(s.size);
        rend.draw_rect(sx, sy, sz, sz, sf::Color(255, 255, 255, alpha));
    }

    // Title - "HELBREATH" above, "XTREME" below with fire effect
    {
        float pulse = 200.0f + 55.0f * std::sin(elapsed_time_ * 1.5f);
        auto title_alpha = static_cast<uint8_t>(std::clamp(pulse, 0.0f, 255.0f));

        // "HELBREATH" - upper line, white with gentle pulse
        constexpr int32_t hb_size = 26;
        float hb_w = rend.text().measure_width("HELBREATH", hb_size);
        int32_t hb_x = (sw - static_cast<int32_t>(hb_w)) / 2;
        int32_t hb_y = sh * 28 / 100;
        rend.draw_text_outlined("HELBREATH", hb_x, hb_y,
                                 sf::Color(255, 255, 255, title_alpha),
                                 sf::Color(0, 0, 0, title_alpha), hb_size, 2.0f);

        // "XTREME" - below, on fire
        constexpr int32_t xt_size = 38;
        float xt_w = rend.text().measure_width("XTREME", xt_size);
        int32_t xt_x = (sw - static_cast<int32_t>(xt_w)) / 2;
        int32_t xt_y = hb_y + hb_size + 8;

        // Warm glow halo
        {
            float glow_pulse = 0.7f + 0.3f * std::sin(elapsed_time_ * 2.0f);
            struct glow_ring { float thickness; uint8_t r, g, b; float base_alpha; };
            static constexpr std::array<glow_ring, 5> rings = {{
                {18.0f, 120,  10,   0,  10.0f},
                {14.0f, 170,  30,   0,  16.0f},
                {10.0f, 220,  70,   0,  22.0f},
                { 6.0f, 245, 130,  10,  30.0f},
                { 3.0f, 255, 180,  40,  40.0f},
            }};
            for (const auto& g : rings)
            {
                auto a = static_cast<uint8_t>(g.base_alpha * glow_pulse);
                rend.draw_text_outlined("XTREME", xt_x, xt_y,
                                         sf::Color(g.r, g.g, g.b, a),
                                         sf::Color(g.r, g.g, g.b, static_cast<uint8_t>(a / 2)),
                                         xt_size, g.thickness);
            }
        }

        // Ember particles rising from text
        struct ember { float x; float speed; float phase; float size; };
        static const std::array<ember, 48> embers = []() {
            std::array<ember, 48> e{};
            for (size_t i = 0; i < 48; ++i)
            {
                uint32_t h = static_cast<uint32_t>(i) * 2654435761u;
                e[i].x = static_cast<float>(h % 1000u) / 1000.0f;
                h = h * 2246822519u;
                e[i].speed = 20.0f + static_cast<float>(h % 40u);
                h = h * 374761393u;
                e[i].phase = static_cast<float>(h % 628u) / 100.0f;
                e[i].size = 1.0f + static_cast<float>(h % 3u);
            }
            return e;
        }();

        for (const auto& e : embers)
        {
            float cycle = std::fmod(elapsed_time_ * e.speed / 50.0f + e.phase, 1.0f);
            float rise_height = 80.0f + e.size * 15.0f;
            float ey = static_cast<float>(xt_y + xt_size / 2) - cycle * rise_height;
            float ex = static_cast<float>(xt_x) + e.x * xt_w
                       + 5.0f * std::sin(elapsed_time_ * 3.0f + e.phase);
            float ea = (1.0f - cycle) * 220.0f;
            uint8_t er = 255;
            uint8_t eg = static_cast<uint8_t>(std::clamp(240.0f * (1.0f - cycle * 0.7f), 0.0f, 255.0f));
            uint8_t eb = static_cast<uint8_t>(std::clamp(100.0f * (1.0f - cycle), 0.0f, 255.0f));
            int32_t esz = static_cast<int32_t>(e.size * (1.0f - cycle * 0.5f));
            if (esz < 1) esz = 1;
            rend.draw_rect(static_cast<int32_t>(ex), static_cast<int32_t>(ey),
                            esz, esz,
                            sf::Color(er, eg, eb, static_cast<uint8_t>(std::clamp(ea, 0.0f, 255.0f))));
        }

        // Fire glow layers behind text
        struct fire_layer { float y_off; float speed; float amp; uint8_t r, g, b, a; };
        static constexpr std::array<fire_layer, 9> flames = {{
            {-16.0f, 2.2f, 7.0f, 100,   5,   0,  25},
            {-12.0f, 2.8f, 6.0f, 140,  15,   0,  40},
            { -9.0f, 3.5f, 5.0f, 180,  35,   0,  55},
            { -6.0f, 4.2f, 4.0f, 210,  60,   0,  75},
            { -4.0f, 5.0f, 3.2f, 235, 100,   0, 100},
            { -2.5f, 3.8f, 2.5f, 250, 145,  10, 130},
            { -1.0f, 4.5f, 1.8f, 255, 190,  30, 160},
            {  0.0f, 2.5f, 1.2f, 255, 220,  60, 190},
            {  1.0f, 3.2f, 0.8f, 255, 240, 100, 140},
        }};

        for (const auto& f : flames)
        {
            float wobble = f.amp * std::sin(elapsed_time_ * f.speed + f.y_off * 0.7f);
            float x_wobble = (f.amp * 0.3f) * std::cos(elapsed_time_ * f.speed * 0.7f + f.y_off);
            int32_t fy = xt_y + static_cast<int32_t>(f.y_off + wobble);
            int32_t fx = xt_x + static_cast<int32_t>(x_wobble);
            rend.draw_text_outlined("XTREME", fx, fy,
                                     sf::Color(f.r, f.g, f.b, f.a),
                                     sf::Color(0, 0, 0, 0), xt_size, 0.0f);
        }

        // Main XTREME text - bright fire core with dark outline
        rend.draw_text_outlined("XTREME", xt_x, xt_y,
                                 sf::Color(255, 230, 80),
                                 sf::Color(160, 30, 0), xt_size, 2.5f);

        // Hot white-yellow pulsing overlay
        float hot = 140.0f + 100.0f * std::sin(elapsed_time_ * 3.5f);
        rend.draw_text_outlined("XTREME", xt_x, xt_y,
                                 sf::Color(255, 255, 200, static_cast<uint8_t>(std::clamp(hot, 0.0f, 240.0f))),
                                 sf::Color(0, 0, 0, 0), xt_size, 0.0f);

        // Secondary hot-white flash
        float hot2 = 60.0f + 60.0f * std::sin(elapsed_time_ * 5.5f + 1.5f);
        rend.draw_text_outlined("XTREME", xt_x, xt_y,
                                 sf::Color(255, 255, 255, static_cast<uint8_t>(std::clamp(hot2, 0.0f, 120.0f))),
                                 sf::Color(0, 0, 0, 0), xt_size, 0.0f);
    }

    // Reconnect button
    int32_t btn_x = (sw - button_width_) / 2;
    int32_t btn_y = sh * 55 / 100;

    bool hovered = mouse_x_ >= btn_x && mouse_x_ <= btn_x + button_width_
                && mouse_y_ >= btn_y && mouse_y_ <= btn_y + button_height_;

    sf::Color btn_bg = hovered ? sf::Color(80, 80, 110) : sf::Color(50, 50, 70);
    sf::Color btn_border = hovered ? sf::Color(140, 140, 180) : sf::Color(100, 100, 130);

    rend.draw_rect(btn_x, btn_y, button_width_, button_height_, btn_bg, true);
    rend.draw_rect(btn_x, btn_y, button_width_, button_height_, btn_border, false);

    // Center "Reconnect" text within button
    constexpr uint32_t btn_text_size = 14;
    const char* btn_label = "Reconnect";
    int32_t text_w = static_cast<int32_t>(std::strlen(btn_label)) * 7;
    int32_t text_x = btn_x + (button_width_ - text_w) / 2;
    int32_t text_y = btn_y + (button_height_ - static_cast<int32_t>(btn_text_size)) / 2;

    rend.draw_text(btn_label, text_x, text_y, sf::Color::White, btn_text_size);

    // Status text below button
    if (!status_text_.empty())
    {
        constexpr uint32_t status_text_size = 14;
        int32_t status_w = static_cast<int32_t>(status_text_.size()) * 7;
        int32_t status_x = (sw - status_w) / 2;
        int32_t status_y = btn_y + button_height_ + 16;

        rend.draw_text_outlined(status_text_, status_x, status_y,
                                sf::Color(180, 180, 180), sf::Color::Black,
                                status_text_size, 1.0f);
    }
}

} // namespace hb
