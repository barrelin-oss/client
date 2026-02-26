#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace hb
{

struct debug_server_config;

// debug_server — WebSocket-based debug probe/action/deferred registry.
//
// Usage:
//   debug_server::start(cfg);                          // in application::initialize
//   debug_server::poll(delta_time);                    // in application::update
//   debug_server::stop();                              // in application::shutdown
//
//   debug_server::register_probe("entity/player", []{ return json{...}; });
//   debug_server::register_action("sprite/save_png", [](json args){ ... });
//   debug_server::register_deferred("player/move_to", [](json args){ ... });
//
// All probe/action/deferred callbacks are invoked on the MAIN THREAD inside poll().
// Zero overhead when disabled — register_* are no-ops if server is not started.

class debug_server
{
public:
    using probe_fn    = std::function<nlohmann::json()>;
    using action_fn   = std::function<nlohmann::json(const nlohmann::json&)>;
    using deferred_fn = std::function<void(const nlohmann::json&)>;

    // Lifecycle — called from application
    static void start(const debug_server_config& cfg);
    static void stop();
    static void poll(float delta_time); // call once per frame from main loop

    // Registration — call from game_state init or wherever state lives
    static void register_probe(std::string_view name, probe_fn fn);
    static void register_action(std::string_view name, action_fn fn);
    static void register_deferred(std::string_view name, deferred_fn fn);

    static bool is_running();
};

} // namespace hb
