#pragma once

#include "entity/entity.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <functional>

namespace hb {

class renderer;
class world;
class sprite_manager;
class sound_manager;

// Entity creation callbacks
using entity_created_callback = std::function<void(entity&)>;
using entity_removed_callback = std::function<void(entity_id)>;

class entity_manager {
public:
    entity_manager() = default;
    ~entity_manager() = default;

    entity_manager(const entity_manager&) = delete;
    entity_manager& operator=(const entity_manager&) = delete;

    // Initialization
    void initialize(sound_manager* sounds = nullptr);
    void set_sound_manager(sound_manager* sounds) { sounds_ = sounds; }
    void shutdown();

    // Entity creation
    entity& create_entity(entity_type type);
    entity& create_entity_with_id(entity_id id, entity_type type);

    // Entity access
    entity* get_entity(entity_id id);
    const entity* get_entity(entity_id id) const;
    bool entity_exists(entity_id id) const;

    // Convenience aliases for handlers
    entity* find(entity_id id) { return get_entity(id); }
    entity& create(entity_id id) { return create_entity_with_id(id, entity_type::character); }
    void destroy(entity_id id) { remove_entity(id); }

    // Entity removal
    void remove_entity(entity_id id);
    void remove_all_entities();
    void remove_entities_of_type(entity_type type);

    // Query entities
    std::vector<entity*> get_entities_of_type(entity_type type);
    std::vector<entity*> get_entities_in_range(int32_t x, int32_t y, int32_t range);
    std::vector<entity*> get_entities_on_tile(int32_t tile_x, int32_t tile_y);
    entity* get_entity_at_screen_pos(int32_t screen_x, int32_t screen_y, int32_t camera_x, int32_t camera_y);
    entity* find_at_tile(int32_t tile_x, int32_t tile_y);  // Returns first non-local-player entity at tile

    // Sprite collision detection
    // Checks if a screen point is within the entity's rendered sprite bounds
    bool is_point_in_entity_sprite(const entity& e, sprite_manager& sprites,
                                   int32_t camera_x, int32_t camera_y,
                                   int32_t mouse_x, int32_t mouse_y) const;

    // Local player
    void set_local_player(entity_id id) { local_player_id_ = id; }
    entity_id local_player_id() const { return local_player_id_; }
    entity* local_player();
    const entity* local_player() const;

    // Update all entities
    void update(float delta_time, world& w, bool local_player_combat_mode);

    // Render all entities (Y-sorted, single pass)
    void render(renderer& rend, sprite_manager& sprites, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y);

    // Get visible entities sorted by Y position (for interleaved rendering)
    std::vector<entity*> get_visible_entities_sorted(renderer& rend, int32_t camera_x, int32_t camera_y);

    // Render a single entity (for interleaved rendering)
    void render_single_entity(renderer& rend, sprite_manager& sprites, entity& e, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y);

    // Global render mode - renders all entities without distance culling
    void set_global_render_mode(bool enabled) { global_render_mode_ = enabled; }
    bool is_global_render_mode() const { return global_render_mode_; }

    // Get an entity's screen bounding rect (for overlap checks)
    // Returns nullopt if the sprite can't be resolved
    std::optional<sf::IntRect> get_entity_screen_bounds(const entity& e, sprite_manager& sprites,
                                                        int32_t camera_x, int32_t camera_y) const;

    // Load character sprites based on appearance data in sprite_component
    void load_character_sprites(entity& ent, sprite_manager& sprites);

    // Callbacks
    void set_entity_created_callback(entity_created_callback cb) { on_created_ = std::move(cb); }
    void set_entity_removed_callback(entity_removed_callback cb) { on_removed_ = std::move(cb); }

    // Statistics
    size_t entity_count() const { return entities_.size(); }
    size_t entity_count_of_type(entity_type type) const;

private:
    void cleanup_removed_entities();
    void update_entity(entity& e, float delta_time, world& w, bool local_player_combat_mode);
    void update_animation(entity& e, float delta_time);
    void update_movement(entity& e, float delta_time, world& w, bool local_player_combat_mode);

    void render_entity(renderer& rend, sprite_manager& sprites, const entity& e, int32_t camera_x, int32_t camera_y, int32_t mouse_x, int32_t mouse_y);
    void render_player_character(renderer& rend, sprite_manager& sprites, const entity& e, int32_t screen_x, int32_t screen_y, const animation_component& a);
    void render_npc_or_monster(renderer& rend, sprite_manager& sprites, const entity& e, int32_t screen_x, int32_t screen_y, const animation_component& a);
    void render_entity_name(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y, bool is_hovered);
    void render_entity_health_bar(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y);

    // Play footstep sound for an entity
    void play_footstep_sound(const entity& e, bool running);

    std::unordered_map<entity_id, std::unique_ptr<entity>> entities_;
    entity_id next_entity_id_ = 1;
    entity_id local_player_id_ = invalid_entity_id;

    entity_created_callback on_created_;
    entity_removed_callback on_removed_;

    // Sound manager for footstep sounds
    sound_manager* sounds_ = nullptr;

    // Global render mode - skip distance culling when true
    bool global_render_mode_ = false;
};

} // namespace hb
