#include "debug/position_store.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

namespace hb::debug
{

using json = nlohmann::json;

position_store& position_store::instance()
{
    static position_store store;
    return store;
}

bool position_store::load(const std::filesystem::path& file)
{
    file_path_ = file;

    if (!std::filesystem::exists(file))
    {
        spdlog::info("Position file not found: {}, starting with empty positions", file.string());
        positions_.clear();
        dirty_ = false;
        return true; // Not an error - just no saved positions yet
    }

    try
    {
        std::ifstream ifs(file);
        if (!ifs.is_open())
        {
            spdlog::error("Failed to open position file: {}", file.string());
            return false;
        }

        json j = json::parse(ifs);

        // Check version
        int version = j.value("version", 1);
        if (version != 1)
        {
            spdlog::warn("Unknown position file version: {}, attempting to load anyway", version);
        }

        // Load positions
        positions_.clear();
        if (j.contains("positions") && j["positions"].is_object())
        {
            for (auto& [key, value] : j["positions"].items())
            {
                if (value.contains("x") && value.contains("y"))
                {
                    point pos;
                    pos.x = value["x"].get<int32_t>();
                    pos.y = value["y"].get<int32_t>();
                    positions_[key] = pos;
                }
            }
        }

        // Store last modified time for hot-reload detection
        last_modified_ = std::filesystem::last_write_time(file);
        dirty_ = false;

        spdlog::info("Loaded {} positions from {}", positions_.size(), file.string());
        return true;
    }
    catch (const json::parse_error& e)
    {
        spdlog::error("Failed to parse position file {}: {}", file.string(), e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to load position file {}: {}", file.string(), e.what());
        return false;
    }
}

bool position_store::save() const
{
    if (file_path_.empty())
    {
        spdlog::error("No file path set for position store");
        return false;
    }
    return save(file_path_);
}

bool position_store::save(const std::filesystem::path& file) const
{
    try
    {
        // Ensure directory exists
        auto parent = file.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent))
        {
            std::filesystem::create_directories(parent);
        }

        json j;
        j["version"] = 1;
        j["positions"] = json::object();

        for (const auto& [id, pos] : positions_)
        {
            j["positions"][id] = {{"x", pos.x}, {"y", pos.y}};
        }

        std::ofstream ofs(file);
        if (!ofs.is_open())
        {
            spdlog::error("Failed to open position file for writing: {}", file.string());
            return false;
        }

        ofs << j.dump(2);
        spdlog::info("Saved {} positions to {}", positions_.size(), file.string());
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to save position file {}: {}", file.string(), e.what());
        return false;
    }
}

std::optional<point> position_store::get(std::string_view id) const
{
    auto it = positions_.find(std::string(id));
    if (it != positions_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void position_store::set(std::string_view id, point pos)
{
    positions_[std::string(id)] = pos;
    dirty_ = true;
}

bool position_store::check_for_changes()
{
    if (file_path_.empty() || !std::filesystem::exists(file_path_))
    {
        return false;
    }

    try
    {
        auto current_time = std::filesystem::last_write_time(file_path_);
        if (current_time > last_modified_)
        {
            spdlog::info("Position file changed, reloading...");

            // Store old positions to detect changes
            auto old_positions = positions_;

            if (load(file_path_))
            {
                // Notify callbacks of any changed positions
                for (const auto& [id, pos] : positions_)
                {
                    auto old_it = old_positions.find(id);
                    if (old_it == old_positions.end() || old_it->second.x != pos.x || old_it->second.y != pos.y)
                    {
                        for (const auto& callback : change_callbacks_)
                        {
                            callback(id, pos);
                        }
                    }
                }
                return true;
            }
        }
    }
    catch (const std::exception& e)
    {
        spdlog::warn("Error checking position file for changes: {}", e.what());
    }

    return false;
}

void position_store::on_position_change(position_change_callback callback)
{
    change_callbacks_.push_back(std::move(callback));
}

void position_store::clear()
{
    positions_.clear();
    dirty_ = true;
}

std::vector<std::string> position_store::all_ids() const
{
    std::vector<std::string> ids;
    ids.reserve(positions_.size());
    for (const auto& [id, _] : positions_)
    {
        ids.push_back(id);
    }
    return ids;
}

} // namespace hb::debug
