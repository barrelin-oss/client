#include "entity/entity_manager.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "world/world.hpp"
#include "world/tile.hpp"
#include "core/constants.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

void entity_manager::initialize() {
    spdlog::info("Entity manager initialized");
}

void entity_manager::shutdown() {
    remove_all_entities();
    spdlog::info("Entity manager shutdown");
}

entity& entity_manager::create_entity(entity_type type) {
    entity_id id = next_entity_id_++;
    return create_entity_with_id(id, type);
}

entity& entity_manager::create_entity_with_id(entity_id id, entity_type type) {
    auto e = std::make_unique<entity>(id, type);

    // Add default components based on type
    switch (type) {
        case entity_type::player:
        case entity_type::character:
            e->add_stats();
            e->add_combat();
            e->add_name();
            e->add_movement();
            break;

        case entity_type::npc:
            e->add_name();
            e->add_npc();
            break;

        case entity_type::monster:
            e->add_stats();
            e->add_combat();
            e->add_name();
            e->add_movement();
            e->add_monster();
            break;

        case entity_type::item:
            e->add_item();
            break;

        case entity_type::effect:
            e->add_effect();
            break;

        default:
            break;
    }

    entity& ref = *e;
    entities_[id] = std::move(e);

    if (on_created_) {
        on_created_(ref);
    }

    spdlog::debug("Created entity {} of type {}", id, static_cast<int>(type));
    return ref;
}

entity* entity_manager::get_entity(entity_id id) {
    auto it = entities_.find(id);
    if (it != entities_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const entity* entity_manager::get_entity(entity_id id) const {
    auto it = entities_.find(id);
    if (it != entities_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool entity_manager::entity_exists(entity_id id) const {
    return entities_.find(id) != entities_.end();
}

void entity_manager::remove_entity(entity_id id) {
    auto it = entities_.find(id);
    if (it != entities_.end()) {
        it->second->mark_for_removal();
    }
}

void entity_manager::remove_all_entities() {
    for (auto& [id, e] : entities_) {
        if (on_removed_) {
            on_removed_(id);
        }
    }
    entities_.clear();
    local_player_id_ = invalid_entity_id;
}

void entity_manager::remove_entities_of_type(entity_type type) {
    for (auto& [id, e] : entities_) {
        if (e->type() == type) {
            e->mark_for_removal();
        }
    }
}

std::vector<entity*> entity_manager::get_entities_of_type(entity_type type) {
    std::vector<entity*> result;
    for (auto& [id, e] : entities_) {
        if (e->type() == type && !e->should_remove()) {
            result.push_back(e.get());
        }
    }
    return result;
}

std::vector<entity*> entity_manager::get_entities_in_range(int32_t x, int32_t y, int32_t range) {
    std::vector<entity*> result;
    int32_t range_sq = range * range;

    for (auto& [id, e] : entities_) {
        if (e->should_remove()) continue;

        const auto& t = e->transform();
        int32_t dx = t.x - x;
        int32_t dy = t.y - y;
        if (dx * dx + dy * dy <= range_sq) {
            result.push_back(e.get());
        }
    }
    return result;
}

std::vector<entity*> entity_manager::get_entities_on_tile(int32_t tile_x, int32_t tile_y) {
    std::vector<entity*> result;
    for (auto& [id, e] : entities_) {
        if (e->should_remove()) continue;

        const auto& t = e->transform();
        if (t.tile_x == tile_x && t.tile_y == tile_y) {
            result.push_back(e.get());
        }
    }
    return result;
}

entity* entity_manager::get_entity_at_screen_pos(int32_t screen_x, int32_t screen_y,
                                                  int32_t camera_x, int32_t camera_y) {
    // Convert screen to world position
    int32_t world_x = screen_x + camera_x;
    int32_t world_y = screen_y + camera_y;

    // Find entity closest to click point
    entity* closest = nullptr;
    int32_t closest_dist_sq = INT32_MAX;

    for (auto& [id, e] : entities_) {
        if (e->should_remove()) continue;
        if (e->type() == entity_type::effect) continue; // Skip effects

        const auto& t = e->transform();
        int32_t dx = t.x - world_x;
        int32_t dy = t.y - world_y;
        int32_t dist_sq = dx * dx + dy * dy;

        // Check if within click radius (32 pixels)
        if (dist_sq < 32 * 32 && dist_sq < closest_dist_sq) {
            closest = e.get();
            closest_dist_sq = dist_sq;
        }
    }

    return closest;
}

entity* entity_manager::local_player() {
    return get_entity(local_player_id_);
}

const entity* entity_manager::local_player() const {
    return get_entity(local_player_id_);
}

void entity_manager::update(float delta_time, world& w) {
    // Update all entities
    for (auto& [id, e] : entities_) {
        if (!e->should_remove()) {
            update_entity(*e, delta_time, w);
        }
    }

    // Clean up removed entities
    cleanup_removed_entities();
}

void entity_manager::cleanup_removed_entities() {
    for (auto it = entities_.begin(); it != entities_.end(); ) {
        if (it->second->should_remove()) {
            if (on_removed_) {
                on_removed_(it->first);
            }
            it = entities_.erase(it);
        } else {
            ++it;
        }
    }
}

void entity_manager::update_entity(entity& e, float delta_time, world& w) {
    // Update animation
    update_animation(e, delta_time);

    // Update movement
    if (e.has_movement()) {
        update_movement(e, delta_time, w);
    }

    // Update combat cooldowns
    if (e.has_combat()) {
        auto& combat = e.combat();
        if (combat.attack_cooldown > 0) {
            combat.attack_cooldown -= delta_time;
        }
        if (combat.spell_cooldown > 0) {
            combat.spell_cooldown -= delta_time;
        }
        if (combat.status_duration > 0) {
            combat.status_duration -= delta_time;
            if (combat.status_duration <= 0) {
                // Clear status effects
                combat.poisoned = false;
                combat.paralyzed = false;
                combat.frozen = false;
            }
        }
    }

    // Update chat bubble timer
    if (e.has_name()) {
        auto& name = e.name();
        if (name.chat_timer > 0) {
            name.chat_timer -= delta_time;
            if (name.chat_timer <= 0) {
                name.chat_message.clear();
            }
        }
    }

    // Update item pickup timer
    if (e.has_item()) {
        auto& item = e.item();
        if (item.pickup_timer > 0) {
            item.pickup_timer -= delta_time;
        }
    }

    // Update effect
    if (e.has_effect()) {
        auto& effect = e.effect();
        effect.effect_timer += delta_time;
        if (effect.effect_timer >= effect.effect_duration) {
            e.mark_for_removal();
        }
    }
}

void entity_manager::update_animation(entity& e, float delta_time) {
    auto& anim = e.animation();

    if (anim.finished && !anim.looping) {
        // Handle state transitions when animation finishes
        switch (anim.state) {
            case entity_anim_state::attack:
            case entity_anim_state::attack_move:
            case entity_anim_state::damage:
            case entity_anim_state::damage_move:
            case entity_anim_state::magic:
            case entity_anim_state::magic_attack:
            case entity_anim_state::get_item:
                // Return to idle after these animations
                anim.set_state(entity_anim_state::stop);
                break;
            case entity_anim_state::dying:
                // Transition to dead state
                anim.set_state(entity_anim_state::dead);
                break;
            default:
                break;
        }
        return;
    }

    anim.frame_timer += delta_time;
    if (anim.frame_timer >= anim.frame_duration) {
        anim.frame_timer = 0.0f;
        anim.current_frame++;

        // Check for attack trigger frame (frame 4 for attacks)
        if (anim.state == entity_anim_state::attack ||
            anim.state == entity_anim_state::attack_move ||
            anim.state == entity_anim_state::magic_attack) {
            if (anim.current_frame == 4 && !anim.attack_triggered) {
                anim.attack_triggered = true;
                // Attack damage would be triggered here through callback
            }
        }

        if (anim.current_frame >= anim.frame_count) {
            if (anim.looping) {
                anim.current_frame = 0;
            } else {
                anim.current_frame = anim.frame_count - 1;
                anim.finished = true;
            }
        }
    }
}

void entity_manager::update_movement(entity& e, float delta_time, world& w) {
    auto& t = e.transform();
    auto& m = e.movement();

    if (!t.moving || !m.can_move) {
        return;
    }

    // Calculate movement speed
    float speed = m.running ? m.run_speed : m.speed;
    t.move_progress += speed * delta_time;

    if (t.move_progress >= 1.0f) {
        // Reached destination tile
        t.tile_x = t.dest_tile_x;
        t.tile_y = t.dest_tile_y;
        t.x = t.tile_x * tile_width + tile_width / 2;
        t.y = t.tile_y * tile_height + tile_height / 2;
        t.move_progress = 0.0f;
        t.moving = false;

        // Check for next waypoint in path
        if (m.path_index < m.path.size()) {
            auto [next_x, next_y] = m.path[m.path_index++];
            if (w.current_map().is_walkable(next_x, next_y)) {
                t.dest_tile_x = next_x;
                t.dest_tile_y = next_y;
                t.moving = true;
            } else {
                m.path.clear();
                m.path_index = 0;
            }
        }
    } else {
        // Interpolate position
        int32_t start_x = t.tile_x * tile_width + tile_width / 2;
        int32_t start_y = t.tile_y * tile_height + tile_height / 2;
        int32_t end_x = t.dest_tile_x * tile_width + tile_width / 2;
        int32_t end_y = t.dest_tile_y * tile_height + tile_height / 2;

        t.x = start_x + static_cast<int32_t>((end_x - start_x) * t.move_progress);
        t.y = start_y + static_cast<int32_t>((end_y - start_y) * t.move_progress);
    }
}

void entity_manager::render(renderer& rend, sprite_manager& sprites, int32_t camera_x, int32_t camera_y) {
    // Collect visible entities and sort by Y position for depth ordering
    std::vector<entity*> visible_entities;

    // Use actual screen dimensions from renderer with margin for off-screen sprites
    // that may extend into view (characters render at -32, -64 from their position)
    static constexpr int32_t render_margin = 128;
    int32_t scr_width = static_cast<int32_t>(rend.width());
    int32_t scr_height = static_cast<int32_t>(rend.height());

    for (auto& [id, e] : entities_) {
        if (e->should_remove()) continue;
        if (!e->sprite().visible) continue;

        // In global render mode, skip distance culling - render all entities
        if (global_render_mode_) {
            visible_entities.push_back(e.get());
            continue;
        }

        const auto& t = e->transform();
        int32_t screen_x = t.x - camera_x;
        int32_t screen_y = t.y - camera_y;

        // Check if on screen (with margin to prevent pop-in/pop-out)
        if (screen_x >= -render_margin && screen_x < scr_width + render_margin &&
            screen_y >= -render_margin && screen_y < scr_height + render_margin) {
            visible_entities.push_back(e.get());
        }
    }

    // Sort by Y position (entities lower on screen render on top)
    std::sort(visible_entities.begin(), visible_entities.end(),
        [](const entity* a, const entity* b) {
            return a->transform().y < b->transform().y;
        });

    // Render sorted entities
    for (entity* e : visible_entities) {
        render_entity(rend, sprites, *e, camera_x, camera_y);
    }
}

void entity_manager::render_entity(renderer& rend, sprite_manager& sprites, const entity& e, int32_t camera_x, int32_t camera_y) {
    const auto& t = e.transform();
    const auto& s = e.sprite();
    const auto& a = e.animation();

    int32_t screen_x = t.x - camera_x;
    int32_t screen_y = t.y - camera_y;

    // Render sprite layers based on entity type
    if (e.type() == entity_type::item) {
        // Items just render their sprite
        if (s.body_sprite) {
            rend.draw_sprite(*s.body_sprite, screen_x - 16, screen_y - 16, s.body_frame);
        }
    } else if (e.type() == entity_type::effect) {
        // Effects
        if (e.has_effect() && e.effect().effect_sprite) {
            const auto& eff = e.effect();
            rend.draw_sprite(*eff.effect_sprite, screen_x + eff.offset_x, screen_y + eff.offset_y, eff.effect_frame);
        }
    } else {
        // Characters, NPCs, monsters - layered rendering
        // Sprite constants (from menu_character_renderer.hpp)
        static constexpr uint16_t body_base = 500;
        static constexpr uint16_t body_stride = 120;
        static constexpr uint16_t male_underwear_base = 4580;
        static constexpr uint16_t female_underwear_base = 14580;
        static constexpr uint16_t underwear_stride = 15;
        static constexpr uint16_t male_hair_base = 4820;
        static constexpr uint16_t female_hair_base = 14820;
        static constexpr uint16_t hair_stride = 15;

        // Direction enum: none=0, north=1, ... north_west=8
        // Sprite IDs use direction 1-8 (dir-1 for 0-7 index)
        int32_t dir = (t.direction == direction::none) ? 5 : static_cast<int32_t>(t.direction);  // Default to south (5) if none

        // Map animation state to action index
        int32_t action = 0;  // Default: stop
        switch (a.state) {
            case entity_anim_state::stop: action = 0; break;
            case entity_anim_state::move: action = 1; break;
            case entity_anim_state::run: action = 2; break;
            case entity_anim_state::attack:
            case entity_anim_state::attack_move: action = 3; break;
            case entity_anim_state::magic:
            case entity_anim_state::magic_attack: action = 4; break;
            case entity_anim_state::get_item: action = 5; break;
            case entity_anim_state::damage:
            case entity_anim_state::damage_move: action = 6; break;
            case entity_anim_state::dying: action = 10; break;
            case entity_anim_state::dead: action = 11; break;
            default: action = 0; break;
        }

        // Calculate frame index for sprites: (dir-1)*8 + current_frame
        int32_t frame_index = (dir - 1) * 8 + a.current_frame;

        // Determine gender and appearance
        uint8_t gender = std::clamp(s.gender, uint8_t(1), uint8_t(2));
        uint8_t skin = std::clamp(s.skin_color, uint8_t(1), uint8_t(3));
        uint8_t hair_style = std::clamp(s.hair_style, uint8_t(0), uint8_t(7));
        uint8_t underwear_color = std::clamp(s.underwear_color, uint8_t(0), uint8_t(7));
        bool is_female = (gender == 2);

        // Calculate owner type: 1-3 for male (skin 1-3), 4-6 for female (skin 1-3)
        int32_t owner_type = is_female ? (3 + skin) : skin;

        // Body sprite ID: 500 + (owner_type - 1) * 120 + action * 8 + (dir - 1)
        uint16_t body_id = static_cast<uint16_t>(body_base + (owner_type - 1) * body_stride + action * 8 + (dir - 1));
        const sprite* body_spr = sprites.get_sprite_by_id(body_id);

        // Underwear sprite ID: base + color * 15 + action
        uint16_t underwear_base = is_female ? female_underwear_base : male_underwear_base;
        uint16_t underwear_id = static_cast<uint16_t>(underwear_base + underwear_color * underwear_stride + action);
        const sprite* underwear_spr = sprites.get_sprite_by_id(underwear_id);

        // Hair sprite ID: base + style * 15 + action
        uint16_t hair_base = is_female ? female_hair_base : male_hair_base;
        uint16_t hair_id = static_cast<uint16_t>(hair_base + hair_style * hair_stride + action);
        const sprite* hair_spr = sprites.get_sprite_by_id(hair_id);

        // Draw layers: underwear, body, hair
        if (underwear_spr) {
            if (s.alpha < 1.0f) {
                rend.draw_sprite_alpha(*underwear_spr, screen_x - 32, screen_y - 64, frame_index, s.alpha);
            } else {
                rend.draw_sprite(*underwear_spr, screen_x - 32, screen_y - 64, frame_index);
            }
        }

        if (body_spr) {
            if (s.alpha < 1.0f) {
                rend.draw_sprite_alpha(*body_spr, screen_x - 32, screen_y - 64, a.current_frame, s.alpha);
            } else {
                rend.draw_sprite(*body_spr, screen_x - 32, screen_y - 64, a.current_frame);
            }
        }

        // Hair (if no helm)
        if (!s.helm_sprite && hair_spr) {
            rend.draw_sprite(*hair_spr, screen_x - 32, screen_y - 64, frame_index);
        }

        // TODO: Armor, helmet, weapon, shield rendering with dynamic lookup

        // Effect overlay (keep using pre-loaded sprite pointer)
        if (s.effect_sprite) {
            rend.draw_sprite(*s.effect_sprite, screen_x - 32, screen_y - 64, a.current_frame);
        }
    }

    // Render name and health bar for characters/monsters
    if (e.has_name() && (e.type() == entity_type::player ||
                         e.type() == entity_type::character ||
                         e.type() == entity_type::npc ||
                         e.type() == entity_type::monster)) {
        render_entity_name(rend, e, screen_x, screen_y);
    }

    if (e.has_stats() && (e.type() == entity_type::character ||
                          e.type() == entity_type::monster)) {
        render_entity_health_bar(rend, e, screen_x, screen_y);
    }
}

void entity_manager::render_entity_name(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y) {
    const auto& name = e.name();

    // Render name above entity
    sf::Color name_color = sf::Color::White;

    if (e.type() == entity_type::npc) {
        name_color = sf::Color::Yellow;
    } else if (e.type() == entity_type::monster) {
        if (e.has_monster() && e.monster().is_boss) {
            name_color = sf::Color::Red;
        } else {
            name_color = sf::Color(255, 128, 0); // Orange
        }
    } else if (e.has_combat() && e.combat().pk_count > 0) {
        name_color = sf::Color::Red;
    }

    // Center name above entity
    int32_t name_x = screen_x - static_cast<int32_t>(name.name.length() * 4);
    int32_t name_y = screen_y - 80;

    rend.draw_text(name.name, name_x, name_y, name_color);

    // Render guild name if present
    if (!name.guild_name.empty()) {
        int32_t guild_x = screen_x - static_cast<int32_t>(name.guild_name.length() * 3);
        rend.draw_text("<" + name.guild_name + ">", guild_x, name_y - 14, sf::Color(100, 200, 100));
    }

    // Render chat bubble
    if (!name.chat_message.empty()) {
        int32_t chat_x = screen_x - static_cast<int32_t>(name.chat_message.length() * 3);
        int32_t chat_y = screen_y - 100;

        // Background
        rend.draw_rect(chat_x - 4, chat_y - 2,
                       static_cast<int32_t>(name.chat_message.length() * 6 + 8), 16,
                       sf::Color(0, 0, 0, 180), true);
        rend.draw_text(name.chat_message, chat_x, chat_y, sf::Color::White);
    }
}

void entity_manager::render_entity_health_bar(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y) {
    const auto& stats = e.stats();

    // Health bar position
    int32_t bar_x = screen_x - 20;
    int32_t bar_y = screen_y - 70;
    int32_t bar_width = 40;
    int32_t bar_height = 4;

    // Background
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(40, 40, 40), true);

    // Health fill
    float hp_ratio = static_cast<float>(stats.hp) / static_cast<float>(stats.max_hp);
    int32_t fill_width = static_cast<int32_t>(bar_width * hp_ratio);

    sf::Color hp_color = sf::Color::Green;
    if (hp_ratio < 0.3f) {
        hp_color = sf::Color::Red;
    } else if (hp_ratio < 0.6f) {
        hp_color = sf::Color::Yellow;
    }

    if (fill_width > 0) {
        rend.draw_rect(bar_x, bar_y, fill_width, bar_height, hp_color, true);
    }

    // Border
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(100, 100, 100), false);
}

void entity_manager::load_character_sprites(entity& ent, sprite_manager& sprites) {
    auto& s = ent.sprite();

    // Sprite ID constants (from menu_character_renderer.hpp)
    static constexpr uint16_t body_base = 500;
    static constexpr uint16_t body_stride = 120;
    static constexpr uint16_t male_underwear_base = 4580;
    static constexpr uint16_t female_underwear_base = 14580;
    static constexpr uint16_t underwear_stride = 15;
    static constexpr uint16_t male_hair_base = 4820;
    static constexpr uint16_t female_hair_base = 14820;
    static constexpr uint16_t hair_stride = 15;

    // Action index for idle/stop (action 0)
    static constexpr int32_t action_stop = 0;

    // Clamp appearance values
    uint8_t gender = std::clamp(s.gender, uint8_t(1), uint8_t(2));
    uint8_t skin = std::clamp(s.skin_color, uint8_t(1), uint8_t(3));
    uint8_t hair_style = std::clamp(s.hair_style, uint8_t(0), uint8_t(7));
    uint8_t underwear_color = std::clamp(s.underwear_color, uint8_t(0), uint8_t(7));

    bool is_female = (gender == 2);

    // Calculate owner type: 1-3 for male (skin 1-3), 4-6 for female (skin 1-3)
    int32_t owner_type = is_female ? (3 + skin) : skin;

    // Body sprite ID: 500 + (owner_type - 1) * 120 + action * 8 + direction
    // For idle, we use action 0, direction will be applied during rendering
    uint16_t body_id = static_cast<uint16_t>(body_base + (owner_type - 1) * body_stride + action_stop * 8);
    s.body_sprite = sprites.get_sprite_by_id(body_id);

    // Underwear sprite ID: base + color * 15 + action
    uint16_t underwear_base = is_female ? female_underwear_base : male_underwear_base;
    uint16_t underwear_id = static_cast<uint16_t>(underwear_base + underwear_color * underwear_stride + action_stop);
    s.underwear_sprite = sprites.get_sprite_by_id(underwear_id);

    // Hair sprite ID: base + style * 15 + action
    uint16_t hair_base = is_female ? female_hair_base : male_hair_base;
    uint16_t hair_id = static_cast<uint16_t>(hair_base + hair_style * hair_stride + action_stop);
    s.hair_sprite = sprites.get_sprite_by_id(hair_id);

    spdlog::debug("Loaded character sprites - body:{} underwear:{} hair:{} (gender:{} skin:{} hair_style:{})",
                  body_id, underwear_id, hair_id, gender, skin, hair_style);

    if (!s.body_sprite) {
        spdlog::warn("Failed to load body sprite ID {}", body_id);
    }
    if (!s.underwear_sprite) {
        spdlog::warn("Failed to load underwear sprite ID {}", underwear_id);
    }
    if (!s.hair_sprite) {
        spdlog::warn("Failed to load hair sprite ID {}", hair_id);
    }
}

size_t entity_manager::entity_count_of_type(entity_type type) const {
    size_t count = 0;
    for (const auto& [id, e] : entities_) {
        if (e->type() == type && !e->should_remove()) {
            ++count;
        }
    }
    return count;
}

} // namespace hb
