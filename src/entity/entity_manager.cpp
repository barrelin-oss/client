#include "entity/entity_manager.hpp"
#include "audio/sound_manager.hpp"
#include "audio/sound_types.hpp"
#include "graphics/renderer.hpp"
#include "assets/sprite_manager.hpp"
#include "world/world.hpp"
#include "world/tile.hpp"
#include "core/constants.hpp"
#include "core/direction_utils.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace hb {

namespace {

// Character sprite constants (from menu_character_renderer.hpp)
struct character_sprite_constants
{
    static constexpr uint16_t body_base = 500;
    static constexpr uint16_t body_stride = 120;
    static constexpr uint16_t male_underwear_base = 4580;
    static constexpr uint16_t female_underwear_base = 14580;
    static constexpr uint16_t underwear_stride = 15;
    static constexpr uint16_t male_hair_base = 4820;
    static constexpr uint16_t female_hair_base = 14820;
    static constexpr uint16_t hair_stride = 15;
};

// NPC/Monster sprite constants
// Legacy formula: 1220 + (type - 10) * 56 + action * 8 + (dir - 1)
// Each NPC/monster type has 56 frames: 7 actions × 8 directions
struct npc_sprite_constants
{
    static constexpr uint16_t npc_base = 1220;
    static constexpr uint16_t npc_type_offset = 10;
    static constexpr uint16_t frames_per_type = 56;  // 7 actions * 8 directions
    static constexpr uint16_t directions_per_action = 8;
};

// Calculate owner type from gender and skin color
// Returns 1-3 for male, 4-6 for female
inline int32_t calculate_owner_type(uint8_t gender, uint8_t skin_color)
{
    uint8_t clamped_gender = std::clamp(gender, uint8_t(1), uint8_t(2));
    uint8_t clamped_skin = std::clamp(skin_color, uint8_t(1), uint8_t(3));
    bool is_female = (clamped_gender == 2);
    return is_female ? (3 + clamped_skin) : clamped_skin;
}

// Calculate body sprite ID for a character
inline uint16_t calculate_body_sprite_id(int32_t owner_type, int32_t action, int32_t direction)
{
    // Body sprite ID: 500 + (owner_type - 1) * 120 + action * 8 + (dir - 1)
    return static_cast<uint16_t>(
        character_sprite_constants::body_base +
        (owner_type - 1) * character_sprite_constants::body_stride +
        action * 8 + (direction - 1));
}

// Calculate underwear sprite ID for a character
inline uint16_t calculate_underwear_sprite_id(bool is_female, uint8_t underwear_color, int32_t action)
{
    uint8_t clamped_color = std::clamp(underwear_color, uint8_t(0), uint8_t(7));
    uint16_t base = is_female ?
        character_sprite_constants::female_underwear_base :
        character_sprite_constants::male_underwear_base;
    return static_cast<uint16_t>(base + clamped_color * character_sprite_constants::underwear_stride + action);
}

// Calculate hair sprite ID for a character
inline uint16_t calculate_hair_sprite_id(bool is_female, uint8_t hair_style, int32_t action)
{
    uint8_t clamped_style = std::clamp(hair_style, uint8_t(0), uint8_t(7));
    uint16_t base = is_female ?
        character_sprite_constants::female_hair_base :
        character_sprite_constants::male_hair_base;
    return static_cast<uint16_t>(base + clamped_style * character_sprite_constants::hair_stride + action);
}

// Map object_action to NPC action index (0-6)
// NPC actions: 0=stop, 1=move, 2=attack, 3=damage, 4=dying, 5=dead, 6=magic
inline int32_t action_to_npc_action_index(object_action action)
{
    switch (action)
    {
        case object_action::stop_peace:
        case object_action::stop_combat:
            return 0;  // Stop/idle
        case object_action::move_peace:
        case object_action::move_combat:
        case object_action::run:
            return 1;  // Move
        case object_action::attack_peace:
        case object_action::attack_combat:
        case object_action::attack_combat_bow:
            return 2;  // Attack
        case object_action::damage:
            return 3;  // Damage
        case object_action::dying:
            return 4;  // Dying
        case object_action::magic:
            return 6;  // Magic
        case object_action::get_item:
        default:
            return 0;  // Default to stop
    }
}

// Calculate NPC/monster sprite ID
// Formula: 1220 + (type - 10) * 56 + action * 8 + (dir - 1)
inline uint16_t calculate_npc_sprite_id(uint16_t npc_type, int32_t action, int32_t dir)
{
    // npc_type is already the visual type (10=Slime, 11=Skeleton, etc.)
    // Clamp direction to valid range (1-8)
    dir = std::clamp(dir, 1, 8);

    return static_cast<uint16_t>(
        npc_sprite_constants::npc_base +
        (npc_type - npc_sprite_constants::npc_type_offset) * npc_sprite_constants::frames_per_type +
        action * npc_sprite_constants::directions_per_action +
        (dir - 1));
}

} // anonymous namespace

void entity_manager::initialize(sound_manager* sounds) {
    sounds_ = sounds;
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

entity* entity_manager::find_at_tile(int32_t tile_x, int32_t tile_y) {
    for (auto& [id, e] : entities_) {
        if (e->should_remove()) continue;
        if (id == local_player_id_) continue;  // Skip local player

        const auto& t = e->transform();
        if (t.tile_x == tile_x && t.tile_y == tile_y) {
            return e.get();
        }
    }
    return nullptr;
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

bool entity_manager::is_point_in_entity_sprite(const entity& e, sprite_manager& sprites,
                                                int32_t camera_x, int32_t camera_y,
                                                int32_t mouse_x, int32_t mouse_y) const {
    const auto& t = e.transform();
    const auto& s = e.sprite();
    const auto& a = e.animation();

    int32_t screen_x = t.x - camera_x;
    int32_t screen_y = t.y - camera_y;

    // Get sprite bounds based on entity type
    if (e.type() == entity_type::item) {
        // Items use a simple 32x32 bounds centered at position
        return (mouse_x >= screen_x - 16 && mouse_x < screen_x + 16 &&
                mouse_y >= screen_y - 16 && mouse_y < screen_y + 16);
    }
    else if (e.type() == entity_type::effect) {
        // Effects are not clickable
        return false;
    }
    else if (e.type() == entity_type::npc || e.type() == entity_type::monster) {
        // NPCs and monsters - use NPC sprite bounds
        int32_t dir = direction_to_sprite_index(t.direction);
        int32_t npc_action = action_to_npc_action_index(e.current_action());

        // Get visual type from component or entity
        uint16_t visual_type = e.visual_type();
        if (visual_type == 0) {
            if (e.has_npc()) {
                visual_type = e.npc().npc_type;
            } else if (e.has_monster()) {
                visual_type = e.monster().monster_type;
            }
        }

        // Default to type 10 (Slime) if no visual type is set
        if (visual_type < 10) {
            visual_type = 10;
        }

        uint16_t sprite_id = calculate_npc_sprite_id(visual_type, npc_action, dir);
        const sprite* npc_spr = sprites.get_sprite_by_id(sprite_id);

        if (npc_spr && npc_spr->has_metadata()) {
            sf::IntRect bounds = npc_spr->get_bounds(screen_x, screen_y, a.current_frame);
            return (mouse_x >= bounds.position.x && mouse_x < bounds.position.x + bounds.size.x &&
                    mouse_y >= bounds.position.y && mouse_y < bounds.position.y + bounds.size.y);
        }

        // Fallback to approximate bounds
        return (mouse_x >= screen_x - 32 && mouse_x < screen_x + 32 &&
                mouse_y >= screen_y - 48 && mouse_y < screen_y + 16);
    }
    else {
        // Player characters - use body sprite bounds
        int32_t dir = direction_to_sprite_index(t.direction);
        int32_t action = static_cast<int32_t>(e.current_action());
        int32_t owner_type = calculate_owner_type(s.gender, s.skin_color);

        uint16_t body_id = calculate_body_sprite_id(owner_type, action, dir);
        const sprite* body_spr = sprites.get_sprite_by_id(body_id);

        if (body_spr && body_spr->has_metadata()) {
            // Get actual sprite bounds for current frame
            sf::IntRect bounds = body_spr->get_bounds(screen_x, screen_y, a.current_frame);
            return (mouse_x >= bounds.position.x && mouse_x < bounds.position.x + bounds.size.x &&
                    mouse_y >= bounds.position.y && mouse_y < bounds.position.y + bounds.size.y);
        }

        // Fallback to approximate 64x64 bounds if sprite not available
        return (mouse_x >= screen_x - 32 && mouse_x < screen_x + 32 &&
                mouse_y >= screen_y - 64 && mouse_y < screen_y);
    }
}

entity* entity_manager::local_player() {
    return get_entity(local_player_id_);
}

const entity* entity_manager::local_player() const {
    return get_entity(local_player_id_);
}

void entity_manager::update(float delta_time, world& w, bool local_player_combat_mode) {
    // Update all entities
    for (auto& [id, e] : entities_) {
        if (!e->should_remove()) {
            update_entity(*e, delta_time, w, local_player_combat_mode);
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

void entity_manager::update_entity(entity& e, float delta_time, world& w, bool local_player_combat_mode) {
    // Update animation
    update_animation(e, delta_time);

    // Update movement
    if (e.has_movement()) {
        update_movement(e, delta_time, w, local_player_combat_mode);
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
        anim.frame_timer -= anim.frame_duration;
        anim.current_frame++;

        // Play footstep sounds on walk/run animations
        // Walk: frames 1 and 3 (alternating footsteps)
        // Run: frames 1 and 3 as well
        if (anim.state == entity_anim_state::move) {
            if (anim.current_frame == 1 || anim.current_frame == 3) {
                play_footstep_sound(e, false);  // Walking
            }
        }
        else if (anim.state == entity_anim_state::run) {
            if (anim.current_frame == 1 || anim.current_frame == 3) {
                play_footstep_sound(e, true);  // Running
            }
        }

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

void entity_manager::update_movement(entity& e, float delta_time, world& w, bool local_player_combat_mode) {
    auto& t = e.transform();
    auto& m = e.movement();

    if (!t.moving || !m.can_move) {
        return;
    }

    // Update facing direction during movement
    if (t.dest_tile_x != t.tile_x || t.dest_tile_y != t.tile_y) {
        if (auto dir = calculate_direction(t.tile_x, t.tile_y, t.dest_tile_x, t.dest_tile_y)) {
            t.direction = *dir;
        }
    }

    // Movement timing based on animation frames (legacy Helbreath system)
    // MOVE: 8 frames @ 70ms = 560ms per tile
    // RUN: 8 frames @ 42ms = 336ms per tile
    float move_time_ms = m.running ? 336.0f : 560.0f;  // Total time in milliseconds
    float move_time_sec = move_time_ms / 1000.0f;       // Convert to seconds

    t.move_progress += delta_time / move_time_sec;

    if (t.move_progress >= 1.0f) {
        // Reached destination tile
        t.tile_x = t.dest_tile_x;
        t.tile_y = t.dest_tile_y;
        t.x = t.tile_x * tile_width + 16;   // Tile center X
        t.y = t.tile_y * tile_height + 16;  // Tile center Y (feet position)
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
                // Return to idle with correct combat stance for local player
                if (e.id() == local_player_id_) {
                    e.set_action_with_combat_mode(object_action::stop_peace, local_player_combat_mode);
                } else {
                    e.set_action(object_action::stop_peace);
                }
            }
        } else {
            // No more path - check if we've reached the final destination
            // For other players/NPCs, destination is stored in movement component from server broadcasts
            bool reached_destination = (m.target_x < 0 || m.target_y < 0) ||
                                       (t.tile_x == m.target_x && t.tile_y == m.target_y);

            if (reached_destination) {
                // Clear the destination
                m.target_x = -1;
                m.target_y = -1;
                // Return to idle with correct combat stance for local player
                if (e.id() == local_player_id_) {
                    e.set_action_with_combat_mode(object_action::stop_peace, local_player_combat_mode);
                } else {
                    e.set_action(object_action::stop_peace);
                }
            }
            // Otherwise, keep moving flag false but don't change animation state
            // The animation will continue looping until next position update arrives
        }
    } else {
        // Interpolate position (use tile center for both X and Y - feet should be at tile center)
        int32_t start_x = t.tile_x * tile_width + 16;
        int32_t start_y = t.tile_y * tile_height + 16;
        int32_t end_x = t.dest_tile_x * tile_width + 16;
        int32_t end_y = t.dest_tile_y * tile_height + 16;

        t.x = start_x + static_cast<int32_t>((end_x - start_x) * t.move_progress);
        t.y = start_y + static_cast<int32_t>((end_y - start_y) * t.move_progress);
    }
}

void entity_manager::render(renderer& rend, sprite_manager& sprites, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y) {
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
        render_entity(rend, sprites, *e, camera_x, camera_y, mouse_x, mouse_y);
    }
}

void entity_manager::render_entity(renderer& rend, sprite_manager& sprites, const entity& e, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y) {
    const auto& t = e.transform();
    const auto& s = e.sprite();
    const auto& a = e.animation();

    int32_t screen_x = t.x - camera_x;
    int32_t screen_y = t.y - camera_y;

    // Check if mouse is hovering over entity using actual sprite bounds
    bool is_hovered = is_point_in_entity_sprite(e, sprites, camera_x, camera_y, mouse_x, mouse_y);


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
    } else if (e.type() == entity_type::npc || e.type() == entity_type::monster) {
        // NPCs and monsters - single sprite rendering
        render_npc_or_monster(rend, sprites, e, screen_x, screen_y, a);
    } else {
        // Player characters - layered rendering
        render_player_character(rend, sprites, e, screen_x, screen_y, a);
    }

    // Render name and health bar for characters/monsters
    if (e.has_name() && (e.type() == entity_type::player ||
                         e.type() == entity_type::character ||
                         e.type() == entity_type::npc ||
                         e.type() == entity_type::monster)) {
        render_entity_name(rend, e, screen_x, screen_y, is_hovered);
    }

    if (e.has_stats() && (e.type() == entity_type::character ||
                          e.type() == entity_type::monster)) {
        render_entity_health_bar(rend, e, screen_x, screen_y);
    }
}

void entity_manager::render_player_character(renderer& rend, sprite_manager& sprites, const entity& e,
                                              int32_t screen_x, int32_t screen_y, const animation_component& a)
{
    const auto& t = e.transform();
    const auto& s = e.sprite();

    int32_t dir = direction_to_sprite_index(t.direction);
    int32_t action = static_cast<int32_t>(e.current_action());

    // Calculate frame index for sprites: (dir-1)*8 + current_frame
    int32_t frame_index = (dir - 1) * 8 + a.current_frame;

    // Calculate sprite IDs using helper functions
    int32_t owner_type = calculate_owner_type(s.gender, s.skin_color);
    bool is_female = (s.gender == 2);

    uint16_t body_id = calculate_body_sprite_id(owner_type, action, dir);
    const sprite* body_spr = sprites.get_sprite_by_id(body_id);

    uint16_t underwear_id = calculate_underwear_sprite_id(is_female, s.underwear_color, action);
    const sprite* underwear_spr = sprites.get_sprite_by_id(underwear_id);

    uint16_t hair_id = calculate_hair_sprite_id(is_female, s.hair_style, action);
    const sprite* hair_spr = sprites.get_sprite_by_id(hair_id);

    // Draw layers: underwear, body, hair
    if (underwear_spr) {
        if (s.alpha < 1.0f) {
            rend.draw_sprite_alpha(*underwear_spr, screen_x, screen_y, frame_index, s.alpha);
        } else {
            rend.draw_sprite(*underwear_spr, screen_x, screen_y, frame_index);
        }
    }

    if (body_spr) {
        if (s.alpha < 1.0f) {
            rend.draw_sprite_alpha(*body_spr, screen_x, screen_y, a.current_frame, s.alpha);
        } else {
            rend.draw_sprite(*body_spr, screen_x, screen_y, a.current_frame);
        }
    }

    // Hair (if no helm)
    if (!s.helm_sprite && hair_spr) {
        rend.draw_sprite(*hair_spr, screen_x, screen_y, frame_index);
    }

    // TODO: Armor, helmet, weapon, shield rendering with dynamic lookup

    // Effect overlay (keep using pre-loaded sprite pointer)
    if (s.effect_sprite) {
        rend.draw_sprite(*s.effect_sprite, screen_x, screen_y, a.current_frame);
    }
}

void entity_manager::render_npc_or_monster(renderer& rend, sprite_manager& sprites, const entity& e,
                                            int32_t screen_x, int32_t screen_y, const animation_component& a)
{
    const auto& t = e.transform();
    const auto& s = e.sprite();

    int32_t dir = direction_to_sprite_index(t.direction);
    int32_t npc_action = action_to_npc_action_index(e.current_action());

    // Get visual type from component or entity
    uint16_t visual_type = e.visual_type();
    if (visual_type == 0) {
        if (e.has_npc()) {
            visual_type = e.npc().npc_type;
        } else if (e.has_monster()) {
            visual_type = e.monster().monster_type;
        }
    }

    // Default to type 10 (Slime) if no visual type is set
    if (visual_type < 10) {
        visual_type = 10;
    }

    uint16_t sprite_id = calculate_npc_sprite_id(visual_type, npc_action, dir);
    const sprite* npc_spr = sprites.get_sprite_by_id(sprite_id);

    if (npc_spr) {
        if (s.alpha < 1.0f) {
            rend.draw_sprite_alpha(*npc_spr, screen_x, screen_y, a.current_frame, s.alpha);
        } else {
            rend.draw_sprite(*npc_spr, screen_x, screen_y, a.current_frame);
        }
    }

    // Effect overlay
    if (s.effect_sprite) {
        rend.draw_sprite(*s.effect_sprite, screen_x, screen_y, a.current_frame);
    }
}

void entity_manager::render_entity_name(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y, bool is_hovered) {
    const auto& name = e.name();

    // Only render name if mouse is hovering
    if (is_hovered) {
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

        // Center name below entity feet
        int32_t name_x = screen_x - static_cast<int32_t>(name.name.length() * 4);
        int32_t name_y = screen_y + 10;

        rend.draw_text(name.name, name_x, name_y, name_color);

        // Render guild name if present (below character name)
        if (!name.guild_name.empty()) {
            int32_t guild_x = screen_x - static_cast<int32_t>(name.guild_name.length() * 3);
            rend.draw_text("<" + name.guild_name + ">", guild_x, name_y + 14, sf::Color(100, 200, 100));
        }
    }

    // Always render chat bubble (above entity, not affected by hover)
    if (!name.chat_message.empty()) {
        int32_t chat_x = screen_x - static_cast<int32_t>(name.chat_message.length() * 3);
        int32_t chat_y = screen_y - 80;

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

    // Health fill (guard against division by zero)
    if (stats.max_hp > 0) {
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
    }

    // Border
    rend.draw_rect(bar_x, bar_y, bar_width, bar_height, sf::Color(100, 100, 100), false);
}

void entity_manager::load_character_sprites(entity& ent, sprite_manager& sprites) {
    auto& s = ent.sprite();

    // Action index for idle/stop (action 0)
    static constexpr int32_t action_stop = 0;

    // Calculate sprite IDs using helper functions
    int32_t owner_type = calculate_owner_type(s.gender, s.skin_color);
    bool is_female = (s.gender == 2);

    // Body sprite ID (direction 1 = north, will be changed during rendering)
    uint16_t body_id = calculate_body_sprite_id(owner_type, action_stop, 1);
    s.body_sprite = sprites.get_sprite_by_id(body_id);

    uint16_t underwear_id = calculate_underwear_sprite_id(is_female, s.underwear_color, action_stop);
    s.underwear_sprite = sprites.get_sprite_by_id(underwear_id);

    uint16_t hair_id = calculate_hair_sprite_id(is_female, s.hair_style, action_stop);
    s.hair_sprite = sprites.get_sprite_by_id(hair_id);

    spdlog::debug("Loaded character sprites - body:{} underwear:{} hair:{} (gender:{} skin:{} hair_style:{})",
                  body_id, underwear_id, hair_id, s.gender, s.skin_color, s.hair_style);

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

void entity_manager::play_footstep_sound(const entity& e, bool running) {
    if (!sounds_) return;

    // Only play footsteps for players and characters
    if (e.type() != entity_type::player && e.type() != entity_type::character) {
        return;
    }

    const auto& t = e.transform();
    character_sound sound = running ? character_sound::run_step : character_sound::walk_step;
    sounds_->play_character_sound_at(sound, t.x, t.y);
}

} // namespace hb
