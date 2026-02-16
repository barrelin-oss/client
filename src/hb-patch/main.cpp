#include "builder.h"
#include "hasher.h"
#include "keygen.h"
#include "manifest.h"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace
{

void print_usage()
{
    std::fputs(
        "hb-patch v0.2.0 - Helbreath asset packager\n"
        "\n"
        "Usage:\n"
        "  hb-patch build --source <dir> --output <dir> [--channel <name>]\n"
        "                 [--key <privkey>] [--game-version <ver>]\n"
        "                 [--platform <linux|windows>]\n"
        "  hb-patch push --source <dir> --target <user@host:/path> [--ssh-key <key>]\n"
        "  hb-patch diff --source <dir> --remote <manifest-url> [--key <pubkey>]\n"
        "  hb-patch keygen --output <dir>\n"
        "  hb-patch sign --manifest <path> --key <privkey>\n"
        "\n"
        "Commands:\n"
        "  build    Scan source directory, generate manifest + compressed files\n"
        "  push     Rsync patch output to a remote server\n"
        "  diff     Compare local source against a remote manifest\n"
        "  keygen   Generate a new Ed25519 signing keypair\n"
        "  sign     Sign an existing manifest with a private key\n"
        "\n"
        "Platform builds:\n"
        "  Without --platform: builds shared assets (configs, data files)\n"
        "    Output: out/{channel}/manifest.json + out/{channel}/files/\n"
        "  With --platform: builds platform-specific files (binaries)\n"
        "    Output: out/{channel}/{platform}/manifest.json + out/{channel}/{platform}/files/\n",
        stderr);
}

auto find_arg(const std::vector<std::string_view>& args, std::string_view name) -> std::string_view
{
    for (size_t i = 0; i + 1 < args.size(); ++i)
    {
        if (args[i] == name)
            return args[i + 1];
    }
    return {};
}

int cmd_build(const std::vector<std::string_view>& args)
{
    auto source = find_arg(args, "--source");
    auto output = find_arg(args, "--output");
    auto channel = find_arg(args, "--channel");
    auto key = find_arg(args, "--key");
    auto game_version = find_arg(args, "--game-version");
    auto platform = find_arg(args, "--platform");

    if (source.empty() || output.empty())
    {
        std::fputs("Error: --source and --output are required\n", stderr);
        return 1;
    }

    hb::patch::build_options opts;
    opts.source = source;
    opts.output = output;
    opts.channel = channel.empty() ? "stable" : std::string(channel);
    opts.game_version = game_version.empty() ? "" : std::string(game_version);
    opts.platform = std::string(platform);
    if (!key.empty())
        opts.key_path = key;

    if (platform.empty())
    {
        std::fprintf(stderr, "Building shared patch: %s -> %s/%s\n",
            std::string(source).c_str(), std::string(output).c_str(), opts.channel.c_str());
    }
    else
    {
        std::fprintf(stderr, "Building %s patch: %s -> %s/%s/%s\n",
            std::string(platform).c_str(), std::string(source).c_str(),
            std::string(output).c_str(), opts.channel.c_str(), std::string(platform).c_str());
    }

    auto result = hb::patch::build(opts);
    if (!result.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", result.error().c_str());
        return 1;
    }

    std::fputs("Done.\n", stderr);
    return 0;
}

int cmd_keygen(const std::vector<std::string_view>& args)
{
    auto output = find_arg(args, "--output");
    if (output.empty())
    {
        std::fputs("Error: --output is required\n", stderr);
        return 1;
    }

    auto keypair = hb::patch::generate_keypair();
    if (!keypair.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", keypair.error().c_str());
        return 1;
    }

    auto write_result = hb::patch::write_keypair(output, keypair.value().first, keypair.value().second);
    if (!write_result.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", write_result.error().c_str());
        return 1;
    }

    std::fprintf(stderr, "Keypair written to %s/\n", std::string(output).c_str());
    std::fprintf(stderr, "  Public:  signing_key.pub\n");
    std::fprintf(stderr, "  Private: signing_key.priv (keep this secret!)\n");
    return 0;
}

int cmd_sign(const std::vector<std::string_view>& args)
{
    auto manifest_path = find_arg(args, "--manifest");
    auto key = find_arg(args, "--key");

    if (manifest_path.empty() || key.empty())
    {
        std::fputs("Error: --manifest and --key are required\n", stderr);
        return 1;
    }

    auto manifest_result = hb::patch::read_manifest(manifest_path);
    if (!manifest_result.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", manifest_result.error().c_str());
        return 1;
    }

    auto key_hex = hb::patch::read_key_file(key);
    if (!key_hex.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", key_hex.error().c_str());
        return 1;
    }

    auto& m = manifest_result.value();
    auto sign_result = hb::patch::sign_manifest(m, key_hex.value());
    if (!sign_result.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", sign_result.error().c_str());
        return 1;
    }

    auto write_result = hb::patch::write_manifest(m, manifest_path);
    if (!write_result.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", write_result.error().c_str());
        return 1;
    }

    std::fprintf(stderr, "Signed manifest: %s\n", std::string(manifest_path).c_str());
    return 0;
}

auto run_command(const std::string& cmd) -> std::pair<int, std::string>
{
    std::string output;
    std::array<char, 4096> buf{};

#ifdef _WIN32
    auto* pipe = _popen(cmd.c_str(), "r");
#else
    auto* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe)
    {
        return {-1, "failed to run command"};
    }

    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
    {
        output += buf.data();
    }

#ifdef _WIN32
    int exit_code = _pclose(pipe);
#else
    int status = pclose(pipe);
    int exit_code = WEXITSTATUS(status);
#endif
    return {exit_code, std::move(output)};
}

int cmd_push(const std::vector<std::string_view>& args)
{
    auto source = find_arg(args, "--source");
    auto target = find_arg(args, "--target");
    auto ssh_key = find_arg(args, "--ssh-key");

    if (source.empty() || target.empty())
    {
        std::fputs("Error: --source and --target are required\n", stderr);
        return 1;
    }

    auto source_str = std::string(source);
    if (source_str.back() != '/')
        source_str += '/';

    auto target_str = std::string(target);
    if (target_str.back() != '/')
        target_str += '/';

    auto cmd = std::string("rsync -avz --delete");
    if (!ssh_key.empty())
    {
        cmd += " -e 'ssh -i " + std::string(ssh_key) + "'";
    }
    cmd += " " + source_str + " " + target_str;

    std::fprintf(stderr, "Pushing: %s -> %s\n", source_str.c_str(), target_str.c_str());
    std::fprintf(stderr, "Running: %s\n", cmd.c_str());

    int rc = std::system(cmd.c_str());
    if (rc != 0)
    {
#ifdef _WIN32
        int exit_code = rc;
#else
        int exit_code = WEXITSTATUS(rc);
#endif
        std::fprintf(stderr, "Error: rsync failed with exit code %d\n", exit_code);
        return 1;
    }

    std::fputs("Push complete.\n", stderr);
    return 0;
}

int cmd_diff(const std::vector<std::string_view>& args)
{
    namespace fs = std::filesystem;

    auto source = find_arg(args, "--source");
    auto remote_url = find_arg(args, "--remote");
    auto key = find_arg(args, "--key");

    if (source.empty() || remote_url.empty())
    {
        std::fputs("Error: --source and --remote are required\n", stderr);
        return 1;
    }

    // Fetch remote manifest via curl
    auto curl_cmd = "curl -sf '" + std::string(remote_url) + "'";
    auto [exit_code, json_str] = run_command(curl_cmd);
    if (exit_code != 0)
    {
        std::fprintf(stderr, "Error: failed to fetch remote manifest from %s\n",
            std::string(remote_url).c_str());
        return 1;
    }

    nlohmann::json remote_json;
    try
    {
        remote_json = nlohmann::json::parse(json_str);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        std::fprintf(stderr, "Error: failed to parse remote manifest: %s\n", e.what());
        return 1;
    }

    auto remote_result = hb::patch::read_manifest_json(remote_json);
    if (!remote_result.has_value())
    {
        std::fprintf(stderr, "Error: %s\n", remote_result.error().c_str());
        return 1;
    }
    auto remote_manifest = std::move(remote_result.value());

    // Verify signature if key provided
    if (!key.empty())
    {
        auto pub_hex = hb::patch::read_key_file(key);
        if (!pub_hex.has_value())
        {
            std::fprintf(stderr, "Error: %s\n", pub_hex.error().c_str());
            return 1;
        }

        if (!hb::patch::verify_manifest(remote_manifest, pub_hex.value()))
        {
            std::fputs("Warning: remote manifest signature verification FAILED\n", stderr);
        }
        else
        {
            std::fputs("Remote manifest signature: OK\n", stderr);
        }
    }

    // Hash local files and compare
    std::error_code ec;
    auto source_path = fs::path(std::string(source));

    if (!fs::is_directory(source_path, ec))
    {
        std::fprintf(stderr, "Error: source is not a directory: %s\n",
            std::string(source).c_str());
        return 1;
    }

    // Build local file map
    hb::patch::file_map local_files;
    for (auto& entry : fs::recursive_directory_iterator(source_path, ec))
    {
        if (!entry.is_regular_file())
            continue;

        auto rel = fs::relative(entry.path(), source_path, ec).generic_string();
        auto hash = hb::patch::sha256_file(entry.path());
        if (!hash.has_value())
        {
            std::fprintf(stderr, "Warning: failed to hash %s: %s\n",
                rel.c_str(), hash.error().c_str());
            continue;
        }

        local_files[rel] = {
            .sha256 = hash.value(),
            .size = entry.file_size(),
        };
    }

    // Compare
    size_t added = 0;
    size_t changed = 0;
    size_t removed = 0;
    size_t unchanged = 0;

    for (auto& [path, local_entry] : local_files)
    {
        auto it = remote_manifest.files.find(path);
        if (it == remote_manifest.files.end())
        {
            std::fprintf(stderr, "  + %s (%lu bytes)\n",
                path.c_str(), static_cast<unsigned long>(local_entry.size));
            ++added;
        }
        else if (it->second.sha256 != local_entry.sha256)
        {
            std::fprintf(stderr, "  ~ %s (%lu -> %lu bytes)\n",
                path.c_str(),
                static_cast<unsigned long>(it->second.size),
                static_cast<unsigned long>(local_entry.size));
            ++changed;
        }
        else
        {
            ++unchanged;
        }
    }

    for (auto& [path, remote_entry] : remote_manifest.files)
    {
        if (!local_files.contains(path))
        {
            std::fprintf(stderr, "  - %s\n", path.c_str());
            ++removed;
        }
    }

    std::fprintf(stderr, "\nSummary: %zu added, %zu changed, %zu removed, %zu unchanged\n",
        added, changed, removed, unchanged);

    if (added == 0 && changed == 0 && removed == 0)
    {
        std::fputs("Up to date.\n", stderr);
    }

    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    std::vector<std::string_view> args(argv, argv + argc);

    if (args.size() < 2)
    {
        print_usage();
        return 1;
    }

    auto command = args[1];

    if (command == "build")
        return cmd_build(args);
    if (command == "push")
        return cmd_push(args);
    if (command == "diff")
        return cmd_diff(args);
    if (command == "keygen")
        return cmd_keygen(args);
    if (command == "sign")
        return cmd_sign(args);
    if (command == "--help" || command == "-h" || command == "help")
    {
        print_usage();
        return 0;
    }

    std::fprintf(stderr, "Unknown command: %s\n\n", std::string(command).c_str());
    print_usage();
    return 1;
}
