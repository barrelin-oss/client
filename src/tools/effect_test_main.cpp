#include "tools/effect_test_app.hpp"
#include <spdlog/spdlog.h>

int main()
{
    spdlog::set_level(spdlog::level::info);

    hb::effect_test_app app;

    if (!app.initialize())
    {
        spdlog::error("Failed to initialize effect test tool");
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}
