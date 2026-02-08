#include "application.hpp"
#include "core/launch_options.hpp"

int main(int argc, char* argv[]) {
    auto opts = hb::parse_args(argc, argv);
    hb::application app;
    return app.run(opts);
}
