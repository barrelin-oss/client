#pragma once

#include <optional>
#include <string>

namespace hb {

struct launch_options
{
    std::optional<std::string> username;
    std::optional<std::string> password;

    bool has_credentials() const
    {
        return username.has_value() && password.has_value();
    }
};

inline launch_options parse_args(int argc, char* argv[])
{
    launch_options opts;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--user" && i + 1 < argc)
        {
            opts.username = argv[++i];
        }
        else if (arg == "--pass" && i + 1 < argc)
        {
            opts.password = argv[++i];
        }
    }
    return opts;
}

} // namespace hb
