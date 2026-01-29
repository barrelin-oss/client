#pragma once

#include "entity/entity.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace hb {

class renderer;
class world;

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
    void initialize();
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

    // Local player
    void set_local_player(entity_id id) { local_player_id_ = id; }
    entity_id local_player_id() const { return local_player_id_; }
    entity* local_player();
    const entity* local_player() const;

    // Update all entities
    void update(float delta_time, world& w);

    // Render all entities
    void render(renderer& rend, int32_t camera_x, int32_t camera_y);

    // Callbacks
    void set_entity_created_callback(entity_created_callback cb) { on_created_ = std::move(cb); }
    void set_entity_removed_callback(entity_removed_callback cb) { on_removed_ = std::move(cb); }

    // Statistics
    size_t entity_count() const { return entities_.size(); }
    size_t entity_count_of_type(entity_type type) const;

private:
    void cleanup_removed_entities();
    void update_entity(entity& e, float delta_time, world& w);
    void update_animation(entity& e, float delta_time);
    void update_movement(entity& e, float delta_time, world& w);

    void render_entity(renderer& rend, const entity& e, int32_t camera_x, int32_t camera_y);
    void render_entity_name(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y);
    void render_entity_health_bar(renderer& rend, const entity& e, int32_t screen_x, int32_t screen_y);

    std::unordered_map<entity_id, std::unique_ptr<entity>> entities_;
    entity_id next_entity_id_ = 1;
    entity_id local_player_id_ = invalid_entity_id;

    entity_created_callback on_created_;
    entity_removed_callback on_removed_;
};

} // namespace hb
