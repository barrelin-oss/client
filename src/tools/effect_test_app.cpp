#include "tools/effect_test_app.hpp"
#include "gameplay/effect_types.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <format>

namespace hb
{

// Effect PAK loading table (same as game_state.cpp)
struct effect_pak_entry
{
    const char* pak_name;
    uint8_t global_start;
    uint8_t sprite_count;
    uint8_t local_start;
};

static constexpr std::array effect_paks = {
    effect_pak_entry{"effect", 0, 10, 0},
    effect_pak_entry{"effect2", 10, 3, 0},
    effect_pak_entry{"effect3", 13, 6, 0},
    effect_pak_entry{"effect4", 19, 5, 0},
    effect_pak_entry{"effect5", 24, 7, 1},
    effect_pak_entry{"CruEffect1", 31, 9, 0},
    effect_pak_entry{"effect6", 40, 5, 0},
    effect_pak_entry{"effect7", 45, 12, 0},
    effect_pak_entry{"effect8", 57, 9, 0},
    effect_pak_entry{"effect9", 66, 21, 0},
    effect_pak_entry{"effect10", 87, 2, 0},
    effect_pak_entry{"effect11", 89, 14, 0},
    effect_pak_entry{"effect11s", 104, 1, 0},
    effect_pak_entry{"effect13", 105, 3, 0},
    effect_pak_entry{"effect12", 148, 4, 0},
};

bool effect_test_app::initialize()
{
    spdlog::info("Effect Test Tool starting...");

    // Renderer
    if (!renderer_.initialize(1024, 768, false))
    {
        spdlog::error("Failed to initialize renderer");
        return false;
    }
    renderer_.load_font("assets/fonts/OpenSans-Regular.ttf");
    renderer_.window().setMouseCursorVisible(true);

    // Audio (optional - effects can play sounds)
    if (audio_.initialize())
    {
        sounds_.initialize(audio_);
    }
    else
    {
        spdlog::warn("Audio initialization failed - effects will be silent");
    }

    // Sprite manager
    sprites_.initialize("assets/");

    // Load effect PAKs
    for (const auto& entry : effect_paks)
    {
        std::string pak_path = std::string("sprites/") + entry.pak_name + ".pak";
        if (!sprites_.load_pak(entry.pak_name, pak_path))
        {
            spdlog::debug("Optional effect PAK not found: {}", pak_path);
        }
    }

    // Tile sprite registry (for map rendering)
    tile_registry_.initialize(sprites_, "sprites/");

    // World
    if (!world_.initialize(tile_registry_))
    {
        spdlog::error("Failed to initialize world");
        return false;
    }
    world_.set_screen_size(1024, 768);
    world_.set_cinematic_mode(true);

    // Load a map
    if (!world_.load_map("middleland"))
    {
        spdlog::warn("Could not load middleland, trying aresden...");
        if (!world_.load_map("aresden"))
        {
            spdlog::error("Failed to load any map");
            return false;
        }
    }

    // Center camera on map
    auto& map = world_.current_map();
    int32_t center_x = (map.width() / 2) * 32;
    int32_t center_y = (map.height() / 2) * 32;
    world_.set_camera_position(center_x - 512, center_y - 384);

    // Effect system
    effects_.initialize(sprites_, sounds_, world_);

    // Magic system
    magic_.initialize();

    // Populate spell list
    all_spells_ = magic_.get_all_spells();
    std::sort(all_spells_.begin(), all_spells_.end(), [](const spell* a, const spell* b) { return a->id < b->id; });

    spdlog::info("Loaded {} spells", all_spells_.size());

    // Apply initial additive intensity
    renderer_.set_additive_intensity(additive_intensity_);

    running_ = true;
    last_frame_time_ = std::chrono::steady_clock::now();

    spdlog::info("Effect Test Tool initialized successfully");
    return true;
}

void effect_test_app::run()
{
    while (running_ && renderer_.is_open())
    {
        auto now = std::chrono::steady_clock::now();
        float delta = std::chrono::duration<float>(now - last_frame_time_).count();
        last_frame_time_ = now;

        // Cap delta time
        if (delta > 0.25f)
            delta = 0.25f;

        process_events();
        update(delta);
        render();

        input_.end_frame();
    }
}

void effect_test_app::shutdown()
{
    effects_.shutdown();
    world_.shutdown();
    sounds_.shutdown();
    audio_.shutdown();
    sprites_.shutdown();
    renderer_.shutdown();
}

void effect_test_app::process_events()
{
    while (auto event = renderer_.window().pollEvent())
    {
        input_.process_event(*event);

        if (event->is<sf::Event::Closed>())
        {
            running_ = false;
        }
        else if (const auto* pressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            int32_t mx = pressed->position.x;
            int32_t my = pressed->position.y;

            // Only handle map clicks outside the control panel area (left 280px)
            if (mx > 280)
            {
                if (pressed->button == sf::Mouse::Button::Left)
                {
                    auto [tx, ty] = world_.screen_to_tile(mx, my);
                    auto& map = world_.current_map();
                    if (tx >= 0 && tx < map.width() && ty >= 0 && ty < map.height())
                    {
                        source_tile_ = {tx, ty};
                    }
                }
                else if (pressed->button == sf::Mouse::Button::Right)
                {
                    auto [tx, ty] = world_.screen_to_tile(mx, my);
                    auto& map = world_.current_map();
                    if (tx >= 0 && tx < map.width() && ty >= 0 && ty < map.height())
                    {
                        dest_tile_ = {tx, ty};
                    }
                }
                else if (pressed->button == sf::Mouse::Button::Middle)
                {
                    world_.start_drag(mx, my);
                }
            }
        }
        else if (const auto* released = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (released->button == sf::Mouse::Button::Middle)
            {
                world_.end_drag();
            }
        }
        else if (const auto* moved = event->getIf<sf::Event::MouseMoved>())
        {
            if (world_.is_dragging())
            {
                world_.update_drag(moved->position.x, moved->position.y);
            }
        }
        else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            switch (key->code)
            {
            case sf::Keyboard::Key::Escape:
                running_ = false;
                break;
            case sf::Keyboard::Key::Space:
                trigger_effect();
                break;
            case sf::Keyboard::Key::Left:
            case sf::Keyboard::Key::Up:
                cycle_spell(-1);
                break;
            case sf::Keyboard::Key::Right:
            case sf::Keyboard::Key::Down:
                cycle_spell(1);
                break;
            case sf::Keyboard::Key::L:
                loop_mode_ = !loop_mode_;
                break;
            case sf::Keyboard::Key::A:
                use_additive_blend_ = !use_additive_blend_;
                break;
            case sf::Keyboard::Key::C:
                source_tile_.reset();
                dest_tile_.reset();
                effects_.clear();
                effect_in_progress_ = false;
                break;
            case sf::Keyboard::Key::Equal: // + key
                additive_intensity_ = std::min(5.0f, additive_intensity_ + 0.1f);
                renderer_.set_additive_intensity(additive_intensity_);
                break;
            case sf::Keyboard::Key::Hyphen: // - key
                additive_intensity_ = std::max(0.0f, additive_intensity_ - 0.1f);
                renderer_.set_additive_intensity(additive_intensity_);
                break;
            case sf::Keyboard::Key::RBracket:
                alpha_override_ = std::min(1.0f, alpha_override_ + 0.05f);
                break;
            case sf::Keyboard::Key::LBracket:
                alpha_override_ = std::max(0.0f, alpha_override_ - 0.05f);
                break;
            default:
                break;
            }
        }
        else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            if (wheel->position.x > 280)
            {
                world_.adjust_zoom(wheel->delta * 0.1f, wheel->position.x, wheel->position.y);
            }
        }
    }
}

void effect_test_app::update(float delta_time)
{
    // Sync render overrides to effect system
    effects_.render_override_.active = true;
    effects_.render_override_.force_additive = use_additive_blend_;
    effects_.render_override_.alpha_multiplier = alpha_override_;

    world_.update(delta_time);
    effects_.update(delta_time);
    sprites_.update_memory(delta_time);

    // Loop mode: auto-restart when effects finish
    if (loop_mode_ && effect_in_progress_ && effects_.active_count() == 0)
    {
        loop_delay_timer_ += delta_time;
        if (loop_delay_timer_ >= loop_delay_)
        {
            loop_delay_timer_ = 0.0f;
            trigger_effect();
        }
    }
}

void effect_test_app::render()
{
    renderer_.begin_frame();

    renderer_.begin_scene();
    world_.apply_zoom_view(renderer_);
    world_.render_terrain(renderer_);
    render_tile_markers();
    effects_.render(renderer_, world_.camera_x(), world_.camera_y());
    world_.reset_zoom_view(renderer_);
    renderer_.end_scene();

    renderer_.begin_ui();
    render_controls();
    render_info_overlay();

    renderer_.end_frame();
}

void effect_test_app::render_tile_markers()
{
    int32_t cam_x = world_.camera_x();
    int32_t cam_y = world_.camera_y();

    if (source_tile_.has_value())
    {
        auto [tx, ty] = source_tile_.value();
        int32_t sx = tx * 32 - cam_x;
        int32_t sy = ty * 32 - cam_y;
        renderer_.draw_rect(sx, sy, 32, 32, sf::Color(0, 255, 0, 60), true);
        renderer_.draw_rect(sx, sy, 32, 32, sf::Color(0, 255, 0, 200), false);
    }

    if (dest_tile_.has_value())
    {
        auto [tx, ty] = dest_tile_.value();
        int32_t sx = tx * 32 - cam_x;
        int32_t sy = ty * 32 - cam_y;
        renderer_.draw_rect(sx, sy, 32, 32, sf::Color(255, 0, 0, 60), true);
        renderer_.draw_rect(sx, sy, 32, 32, sf::Color(255, 0, 0, 200), false);
    }
}

void effect_test_app::render_controls()
{
    // Background panel
    renderer_.draw_rect(0, 0, 280, 768, sf::Color(20, 20, 30, 220), true);
    renderer_.draw_line(280, 0, 280, 768, sf::Color(60, 60, 80));

    int32_t y = 10;
    auto color_white = sf::Color::White;
    auto color_grey = sf::Color(180, 180, 180);
    auto color_cyan = sf::Color(100, 200, 255);
    auto color_green = sf::Color(100, 255, 100);

    renderer_.draw_text("EFFECT TEST TOOL", 10, y, color_cyan, 16);
    y += 30;

    // Current spell
    renderer_.draw_text("Spell:", 10, y, color_grey, 12);
    y += 18;
    if (!all_spells_.empty() && spell_index_ >= 0 && spell_index_ < static_cast<int32_t>(all_spells_.size()))
    {
        const auto* sp = all_spells_[spell_index_];
        auto label = std::format("[{}] {}", sp->id, sp->name);
        renderer_.draw_text(label, 10, y, color_white, 14);
        y += 20;
        auto info = std::format("Circle {} | Proj:{} Impact:{}", sp->circle, sp->projectile_effect, sp->effect_sprite);
        renderer_.draw_text(info, 10, y, color_grey, 11);
    }
    else
    {
        renderer_.draw_text("(no spells loaded)", 10, y, color_grey, 12);
    }
    y += 24;

    renderer_.draw_line(10, y, 270, y, sf::Color(60, 60, 80));
    y += 10;

    // Blend settings
    renderer_.draw_text("Blend Settings:", 10, y, color_cyan, 13);
    y += 20;

    auto additive_label = std::format("Additive: {}  [A]", use_additive_blend_ ? "ON" : "OFF");
    renderer_.draw_text(additive_label, 10, y, use_additive_blend_ ? color_green : color_grey, 12);
    y += 18;

    auto intensity_label = std::format("Intensity: {:.1f}  [+/-]", additive_intensity_);
    renderer_.draw_text(intensity_label, 10, y, color_white, 12);
    y += 18;

    auto alpha_label = std::format("Alpha: {:.2f}  [ [ / ] ]", alpha_override_);
    renderer_.draw_text(alpha_label, 10, y, color_white, 12);
    y += 24;

    renderer_.draw_line(10, y, 270, y, sf::Color(60, 60, 80));
    y += 10;

    // Loop mode
    auto loop_label = std::format("Loop: {}  [L]", loop_mode_ ? "ON" : "OFF");
    renderer_.draw_text(loop_label, 10, y, loop_mode_ ? color_green : color_grey, 12);
    y += 24;

    renderer_.draw_line(10, y, 270, y, sf::Color(60, 60, 80));
    y += 10;

    // Active effects count
    auto count_label = std::format("Active effects: {}", effects_.active_count());
    renderer_.draw_text(count_label, 10, y, color_white, 12);
    y += 24;

    renderer_.draw_line(10, y, 270, y, sf::Color(60, 60, 80));
    y += 10;

    // Tile selection info
    renderer_.draw_text("Selection:", 10, y, color_cyan, 13);
    y += 20;

    if (source_tile_.has_value())
    {
        auto [tx, ty] = source_tile_.value();
        auto src_label = std::format("Source: ({}, {})", tx, ty);
        renderer_.draw_text(src_label, 10, y, sf::Color(100, 255, 100), 12);
    }
    else
    {
        renderer_.draw_text("Source: (not set)", 10, y, color_grey, 12);
    }
    y += 18;

    if (dest_tile_.has_value())
    {
        auto [tx, ty] = dest_tile_.value();
        auto dst_label = std::format("Dest: ({}, {})", tx, ty);
        renderer_.draw_text(dst_label, 10, y, sf::Color(255, 100, 100), 12);
    }
    else
    {
        renderer_.draw_text("Dest: (not set)", 10, y, color_grey, 12);
    }
    y += 24;

    renderer_.draw_line(10, y, 270, y, sf::Color(60, 60, 80));
    y += 10;

    // Controls help
    renderer_.draw_text("Controls:", 10, y, color_cyan, 13);
    y += 20;

    const char* controls[] = {
        "Left-click:   Set source tile",
        "Right-click:  Set dest tile",
        "Middle-drag:  Pan camera",
        "Scroll:       Zoom",
        "Space:        Trigger effect",
        "Left/Right:   Change spell",
        "A:            Toggle additive",
        "+/-:          Adjust intensity",
        "[/]:          Adjust alpha",
        "L:            Toggle loop",
        "C:            Clear selection",
        "Esc:          Quit",
    };
    for (const auto* ctrl : controls)
    {
        renderer_.draw_text(ctrl, 10, y, color_grey, 10);
        y += 14;
    }
}

void effect_test_app::render_info_overlay()
{
    // Mouse position info at bottom of screen
    int32_t mx = input_.mouse_x();
    int32_t my = input_.mouse_y();

    if (mx > 280)
    {
        auto [tx, ty] = world_.screen_to_tile(mx, my);
        auto label = std::format("Tile: ({}, {})  Mouse: ({}, {})", tx, ty, mx, my);
        renderer_.draw_text_outlined(label, 290, 748, sf::Color::White, sf::Color::Black, 11, 1.0f);
    }
}

void effect_test_app::trigger_effect()
{
    if (!source_tile_.has_value() || !dest_tile_.has_value())
    {
        spdlog::warn("Select both source and dest tiles first");
        return;
    }

    if (all_spells_.empty() || spell_index_ < 0 || spell_index_ >= static_cast<int32_t>(all_spells_.size()))
    {
        spdlog::warn("No spell selected");
        return;
    }

    const spell* sp = all_spells_[spell_index_];

    // Apply blend settings
    renderer_.set_additive_intensity(additive_intensity_);

    // Convert tiles to world pixels (center of tile)
    auto [src_tx, src_ty] = source_tile_.value();
    auto [dst_tx, dst_ty] = dest_tile_.value();
    float src_wx = src_tx * 32.0f + 16.0f;
    float src_wy = src_ty * 32.0f + 16.0f;
    float dst_wx = dst_tx * 32.0f + 16.0f;
    float dst_wy = dst_ty * 32.0f + 16.0f;

    spdlog::info("Triggering spell '{}' (id={}) proj={} impact={} from ({},{}) to ({},{})",
                 sp->name,
                 sp->id,
                 sp->projectile_effect,
                 sp->effect_sprite,
                 src_tx,
                 src_ty,
                 dst_tx,
                 dst_ty);

    // Trigger projectile (source -> dest)
    if (sp->projectile_effect > 0)
    {
        effects_.add_effect_world(static_cast<effect_type_id>(sp->projectile_effect), src_wx, src_wy, dst_wx, dst_wy);
    }

    // Trigger impact effect (at dest)
    if (sp->effect_sprite > 0)
    {
        effects_.add_effect_at_pixel(static_cast<effect_type_id>(sp->effect_sprite), dst_wx, dst_wy);
    }

    // If neither, try using spell_id as direct effect type for testing
    if (sp->projectile_effect == 0 && sp->effect_sprite == 0)
    {
        spdlog::info("Spell has no effects, using spell_id {} as direct effect type", sp->id);
        effects_.add_effect_world(static_cast<effect_type_id>(sp->id), src_wx, src_wy, dst_wx, dst_wy);
    }

    effect_in_progress_ = true;
    loop_delay_timer_ = 0.0f;
}

void effect_test_app::cycle_spell(int direction)
{
    if (all_spells_.empty())
        return;

    spell_index_ += direction;
    if (spell_index_ < 0)
        spell_index_ = static_cast<int32_t>(all_spells_.size()) - 1;
    if (spell_index_ >= static_cast<int32_t>(all_spells_.size()))
        spell_index_ = 0;
}

} // namespace hb
