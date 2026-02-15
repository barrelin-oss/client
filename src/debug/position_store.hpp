#pragma once

#include "debug/positionable.hpp"
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hb::debug
{

// Callback for position changes (used for hot-reload notifications)
using position_change_callback = std::function<void(std::string_view id, point pos)>;

// Singleton for storing and persisting UI element positions
class position_store
{
public:
    static position_store& instance();

    // Non-copyable
    position_store(const position_store&) = delete;
    position_store& operator=(const position_store&) = delete;

    // Load positions from JSON file
    bool load(const std::filesystem::path& file);

    // Save positions to file (uses path from last load)
    bool save() const;

    // Save positions to specific path
    bool save(const std::filesystem::path& file) const;

    // Get position for an element (returns nullopt if not found)
    std::optional<point> get(std::string_view id) const;

    // Set position for an element
    void set(std::string_view id, point pos);

    // Check if file has changed and reload if necessary (for hot-reload)
    // Returns true if positions were reloaded
    bool check_for_changes();

    // Register callback for position changes during hot-reload
    void on_position_change(position_change_callback callback);

    // Clear all positions
    void clear();

    // Get all position IDs
    std::vector<std::string> all_ids() const;

    // Check if store has been modified since last save
    bool is_dirty() const { return dirty_; }

private:
    position_store() = default;

    std::unordered_map<std::string, point> positions_;
    std::filesystem::path file_path_;
    std::filesystem::file_time_type last_modified_;
    std::vector<position_change_callback> change_callbacks_;
    bool dirty_ = false;
};

} // namespace hb::debug
