#include "Args.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace fs = std::filesystem;

namespace {

constexpr std::array kBoolFlags = {
    "-h"sv,        "--help"sv,       "--full-info"sv,   "--short"sv,
    "--console"sv, "--no-console"sv, "--json"sv,        "--no-json"sv,
    "--yaml"sv,    "--no-yaml"sv,    "--all-recipes"sv, "--cookable"sv};

constexpr std::array kValueFlags = {"--json-out"sv, "--yaml-out"sv,
                                    "--db-path"sv};

[[nodiscard]] std::pair<std::string_view, std::string_view> SplitOption(
    std::string_view token) noexcept {
    const size_t equals_pos = token.find('=');
    if (equals_pos == std::string_view::npos) {
        return {token, {}};
    }
    return {token.substr(0, equals_pos), token.substr(equals_pos + 1)};
}

[[nodiscard]] bool ValidateToken(std::string_view token) {
    if (!token.starts_with('-')) {
        return false;
    }
    const auto [option, value] = SplitOption(token);
    if (std::ranges::contains(kValueFlags, option)) {
        return value.empty();
    }
    if (!value.empty()) {
        return true;
    }
    return !std::ranges::contains(kBoolFlags, option);
}

}  // namespace

namespace io {

bool Args::ShowHelp() const noexcept { return show_help_; }

const AppArgs& Args::App() const noexcept { return app_; }

AppArgs& Args::AppMutable() noexcept { return app_; }

const ArgsOut& Args::Out() const noexcept { return out_; }

ArgsOut& Args::OutMutable() noexcept { return out_; }

bool HasAnyOutput(const ArgsOut& args) noexcept {
    return args.write_console || args.write_json || args.write_yaml;
}

std::expected<Args, std::string> ParseArgs(
    std::span<const std::string_view> argv) {
    Args parsed;
    if (argv.empty()) {
        return parsed;
    }

    for (size_t i = 1; i != argv.size(); ++i) {
        const std::string_view token = argv[i];
        if (ValidateToken(token)) {
            return std::unexpected("Unknown argument: "s + std::string(token));
        }

        const auto [option, value] = SplitOption(token);

        if (option == "-h"sv || option == "--help"sv) {
            parsed.show_help_ = true;
            continue;
        }
        if (option == "--full-info"sv) {
            parsed.out_.is_full_info = true;
            continue;
        }
        if (option == "--short"sv) {
            parsed.out_.is_full_info = false;
            continue;
        }
        if (option == "--console"sv) {
            parsed.out_.write_console = true;
            continue;
        }
        if (option == "--no-console"sv) {
            parsed.out_.write_console = false;
            continue;
        }
        if (option == "--json"sv) {
            parsed.out_.write_json = true;
            continue;
        }
        if (option == "--no-json"sv) {
            parsed.out_.write_json = false;
            continue;
        }
        if (option == "--yaml"sv) {
            parsed.out_.write_yaml = true;
            continue;
        }
        if (option == "--no-yaml"sv) {
            parsed.out_.write_yaml = false;
            continue;
        }
        if (option == "--all-recipes"sv) {
            parsed.app_.recipe_selection = RecipeSelection::All;
            continue;
        }
        if (option == "--cookable"sv) {
            parsed.app_.recipe_selection = RecipeSelection::Cookable;
            continue;
        }
        if (option == "--json-out"sv) {
            parsed.out_.json_out_path = fs::path{value};
            continue;
        }
        if (option == "--yaml-out"sv) {
            parsed.out_.yaml_out_path = fs::path{value};
            continue;
        }
        if (option == "--db-path"sv) {
            parsed.app_.db_path = fs::path{value};
            continue;
        }
    }

    return parsed;
}

std::string BuildUsage(std::string_view program_name) {
    std::string usage;
    usage += "Usage: ";
    usage += std::string(program_name);
    usage += " [options]\n";
    usage += "Options:\n";
    usage += "  -h, --help               Show this help message\n";
    usage +=
        "  --full-info              Print full recipe info to console "
        "(default)\n";
    usage += "  --short                  Print short recipe info to console\n";
    usage += "  --all-recipes            Select all recipes (default)\n";
    usage += "  --cookable               Select only cookable recipes\n";
    usage += "  --console                Enable console output (default)\n";
    usage += "  --no-console             Disable console output\n";
    usage += "  --json                   Enable JSON output (default)\n";
    usage += "  --no-json                Disable JSON output\n";
    usage += "  --yaml                   Enable YAML output (default)\n";
    usage += "  --no-yaml                Disable YAML output\n";
    usage += "  --json-out=<path>        JSON output path\n";
    usage += "  --yaml-out=<path>        YAML output path\n";
    usage += "  --db-path=<path>         SQL database path\n";
    return usage;
}

}  // namespace io
