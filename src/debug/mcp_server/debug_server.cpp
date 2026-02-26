#include "debug/mcp_server/debug_server.hpp"
#include "core/config.hpp"
#include <spdlog/spdlog.h>
#include <ixwebsocket/IXWebSocketServer.h>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace hb
{

namespace
{

struct pending_request
{
    std::string connection_id;
    std::string id;
    std::string type;   // "probe" | "action" | "deferred"
    std::string name;
    nlohmann::json args;
    std::string capture_probe;
    float capture_after_ms = 0.0f;
};

struct deferred_entry
{
    std::string connection_id;
    std::string id;
    std::string capture_probe;
    float remaining_ms;
};

struct server_state
{
    bool running = false;
    std::unique_ptr<ix::WebSocketServer> ws_server;

    std::mutex connections_mutex;
    std::unordered_map<std::string, std::weak_ptr<ix::WebSocket>> connections;

    std::mutex queue_mutex;
    std::deque<pending_request> queue;

    std::unordered_map<std::string, debug_server::probe_fn>    probes;
    std::unordered_map<std::string, debug_server::action_fn>   actions;
    std::unordered_map<std::string, debug_server::deferred_fn> deferreds;

    std::vector<deferred_entry> deferred_entries;
};

server_state& get_state()
{
    static server_state s;
    return s;
}

void send_to(const std::string& connection_id, const nlohmann::json& msg)
{
    auto& s = get_state();
    std::lock_guard lock(s.connections_mutex);
    auto it = s.connections.find(connection_id);
    if (it == s.connections.end()) return;
    auto ws = it->second.lock();
    if (!ws) return;
    ws->send(msg.dump());
}

} // anonymous namespace

void debug_server::start(const debug_server_config& cfg)
{
    if (!cfg.enabled) return;

    auto& s = get_state();
    if (s.running)
    {
        spdlog::warn("debug_server: already running");
        return;
    }

    s.ws_server = std::make_unique<ix::WebSocketServer>(cfg.port, "127.0.0.1");

    s.ws_server->setOnConnectionCallback(
        [&s](std::weak_ptr<ix::WebSocket> weak_ws,
             std::shared_ptr<ix::ConnectionState> connection_state)
        {
            auto ws = weak_ws.lock();
            if (!ws) return;

            std::string conn_id = connection_state->getId();

            {
                std::lock_guard lock(s.connections_mutex);
                s.connections[conn_id] = weak_ws;
            }

            spdlog::info("debug_server: client connected ({})", conn_id);

            ws->setOnMessageCallback(
                [&s, conn_id](const ix::WebSocketMessagePtr& msg)
                {
                    if (msg->type == ix::WebSocketMessageType::Close)
                    {
                        std::lock_guard lock(s.connections_mutex);
                        s.connections.erase(conn_id);
                        spdlog::info("debug_server: client disconnected ({})", conn_id);
                        return;
                    }

                    if (msg->type != ix::WebSocketMessageType::Message) return;

                    nlohmann::json req;
                    try
                    {
                        req = nlohmann::json::parse(msg->str);
                    }
                    catch (const nlohmann::json::exception& e)
                    {
                        spdlog::warn("debug_server: bad JSON from {}: {}", conn_id, e.what());
                        return;
                    }

                    pending_request pr;
                    pr.connection_id    = conn_id;
                    pr.id               = req.value("id", "");
                    pr.type             = req.value("type", "");
                    pr.name             = req.value("name", req.value("action", ""));
                    pr.args             = req.value("args", nlohmann::json::object());
                    pr.capture_probe    = req.value("capture", "");
                    pr.capture_after_ms = req.value("capture_after_ms", 0.0f);

                    std::lock_guard lock(s.queue_mutex);
                    s.queue.push_back(std::move(pr));
                });
        });

    auto [ok, err] = s.ws_server->listen();
    if (!ok)
    {
        spdlog::error("debug_server: failed to listen on port {}: {}", cfg.port, err);
        s.ws_server.reset();
        return;
    }

    s.ws_server->start();
    s.running = true;
    spdlog::info("debug_server: listening on ws://localhost:{}", cfg.port);
}

void debug_server::stop()
{
    auto& s = get_state();
    if (!s.running) return;
    s.ws_server->stop();
    s.ws_server.reset();
    s.running = false;
    spdlog::info("debug_server: stopped");
}

bool debug_server::is_running()
{
    return get_state().running;
}

void debug_server::register_probe(std::string_view name, probe_fn fn)
{
    get_state().probes[std::string(name)] = std::move(fn);
}

void debug_server::register_action(std::string_view name, action_fn fn)
{
    get_state().actions[std::string(name)] = std::move(fn);
}

void debug_server::register_deferred(std::string_view name, deferred_fn fn)
{
    get_state().deferreds[std::string(name)] = std::move(fn);
}

void debug_server::poll(float delta_time)
{
    auto& s = get_state();
    if (!s.running) return;

    std::deque<pending_request> batch;
    {
        std::lock_guard lock(s.queue_mutex);
        std::swap(batch, s.queue);
    }

    for (auto& req : batch)
    {
        if (req.type == "probe")
        {
            auto it = s.probes.find(req.name);
            if (it == s.probes.end())
            {
                send_to(req.connection_id,
                    {{"id", req.id}, {"ok", false}, {"error", "unknown probe: " + req.name}});
                continue;
            }
            nlohmann::json result;
            try { result = it->second(); }
            catch (const std::exception& e) { result = {{"exception", e.what()}}; }
            send_to(req.connection_id, {{"id", req.id}, {"ok", true}, {"result", result}});
        }
        else if (req.type == "action")
        {
            auto it = s.actions.find(req.name);
            if (it == s.actions.end())
            {
                send_to(req.connection_id,
                    {{"id", req.id}, {"ok", false}, {"error", "unknown action: " + req.name}});
                continue;
            }
            nlohmann::json result;
            try { result = it->second(req.args); }
            catch (const std::exception& e) { result = {{"exception", e.what()}}; }
            send_to(req.connection_id, {{"id", req.id}, {"ok", true}, {"result", result}});
        }
        else if (req.type == "deferred")
        {
            auto it = s.deferreds.find(req.name);
            if (it == s.deferreds.end())
            {
                send_to(req.connection_id,
                    {{"id", req.id}, {"ok", false}, {"error", "unknown deferred: " + req.name}});
                continue;
            }
            try { it->second(req.args); }
            catch (const std::exception& e)
            {
                send_to(req.connection_id,
                    {{"id", req.id}, {"ok", false}, {"error", e.what()}});
                continue;
            }
            send_to(req.connection_id, {{"id", req.id}, {"status", "scheduled"}});
            s.deferred_entries.push_back({
                req.connection_id,
                req.id,
                req.capture_probe,
                req.capture_after_ms
            });
        }
        else
        {
            send_to(req.connection_id,
                {{"id", req.id}, {"ok", false}, {"error", "unknown type: " + req.type}});
        }
    }

    float delta_ms = delta_time * 1000.0f;
    auto it = s.deferred_entries.begin();
    while (it != s.deferred_entries.end())
    {
        it->remaining_ms -= delta_ms;
        if (it->remaining_ms <= 0.0f)
        {
            nlohmann::json result;
            auto probe_it = s.probes.find(it->capture_probe);
            if (probe_it != s.probes.end())
            {
                try { result = probe_it->second(); }
                catch (const std::exception& e) { result = {{"exception", e.what()}}; }
                send_to(it->connection_id,
                    {{"id", it->id}, {"type", "result"}, {"ok", true}, {"result", result}});
            }
            else
            {
                send_to(it->connection_id,
                    {{"id", it->id}, {"type", "result"}, {"ok", false},
                     {"error", "unknown capture probe: " + it->capture_probe}});
            }
            it = s.deferred_entries.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace hb
