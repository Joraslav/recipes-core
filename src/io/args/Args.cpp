#include "Args.hpp"

#include "types/kitchen/Types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace fs = std::filesystem;

using namespace io::arg;
using types::RecipeSelection;

namespace {

struct FlagInfo final {
    std::string_view
        name;  // NOLINT(misc-non-private-member-variables-in-classes)
    enum class Type : uint8_t {
        Bool,
        Value,
        Command
    } type;  // NOLINT(misc-non-private-member-variables-in-classes)

    constexpr FlagInfo(std::string_view p_name, Type p_type) noexcept
        : name(p_name), type(p_type) {}
};

constexpr std::array kFlags = {
    FlagInfo{"-h"sv, FlagInfo::Type::Bool},
    FlagInfo{"--help"sv, FlagInfo::Type::Bool},
    FlagInfo{"--full-info"sv, FlagInfo::Type::Bool},
    FlagInfo{"--short"sv, FlagInfo::Type::Bool},
    FlagInfo{"--console"sv, FlagInfo::Type::Bool},
    FlagInfo{"--no-console"sv, FlagInfo::Type::Bool},
    FlagInfo{"--json"sv, FlagInfo::Type::Bool},
    FlagInfo{"--no-json"sv, FlagInfo::Type::Bool},
    FlagInfo{"--yaml"sv, FlagInfo::Type::Bool},
    FlagInfo{"--no-yaml"sv, FlagInfo::Type::Bool},
    FlagInfo{"--all-recipes"sv, FlagInfo::Type::Bool},
    FlagInfo{"--cookable"sv, FlagInfo::Type::Bool},

    FlagInfo{"--json-out"sv, FlagInfo::Type::Value},
    FlagInfo{"--yaml-out"sv, FlagInfo::Type::Value},
    FlagInfo{"--db-path"sv, FlagInfo::Type::Value},

    FlagInfo{"--add-products"sv, FlagInfo::Type::Command},
    FlagInfo{"--add-recipes"sv, FlagInfo::Type::Command},
    FlagInfo{"--list-products"sv, FlagInfo::Type::Command},
    FlagInfo{"--list-recipes"sv, FlagInfo::Type::Command},
};

[[nodiscard]] std::optional<FlagInfo::Type> GetFlagType(
    std::string_view option) noexcept {
    const auto* it = std::ranges::find(kFlags, option, &FlagInfo::name);
    if (it != kFlags.end()) {
        return it->type;
    }
    return std::nullopt;
}

[[nodiscard]] std::pair<std::string_view, std::string_view> SplitOption(
    std::string_view token) noexcept {
    const size_t equals_pos = token.find('=');
    if (equals_pos == std::string_view::npos) {
        return {token, {}};
    }
    return {token.substr(0, equals_pos), token.substr(equals_pos + 1)};
}

[[nodiscard]] bool IsValidToken(std::string_view token) {
    if (!token.starts_with('-')) {
        return false;
    }
    const auto [option, value] = SplitOption(token);
    const auto type = GetFlagType(option);
    if (!type.has_value()) {
        return false;
    }

    switch (*type) {
        case FlagInfo::Type::Value:
            return !value.empty();
        case FlagInfo::Type::Bool:
        case FlagInfo::Type::Command:
            return value.empty();
        default:
            return false;
    }
}

void SetBoolFlag(Args& args, std::string_view option) noexcept {
    if (option == "-h"sv || option == "--help"sv) {
        args.SetHelp(true);
    }
    if (option == "--full-info"sv) {
        args.OutMutable().is_full_info = true;
    }
    if (option == "--short"sv) {
        args.OutMutable().is_full_info = false;
    }
    if (option == "--console"sv) {
        args.OutMutable().write_console = true;
    }
    if (option == "--no-console"sv) {
        args.OutMutable().write_console = false;
    }
    if (option == "--json"sv) {
        args.OutMutable().write_json = true;
    }
    if (option == "--no-json"sv) {
        args.OutMutable().write_json = false;
    }
    if (option == "--yaml"sv) {
        args.OutMutable().write_yaml = true;
    }
    if (option == "--no-yaml"sv) {
        args.OutMutable().write_yaml = false;
    }
    if (option == "--all-recipes"sv) {
        args.AppMutable().recipe_selection = RecipeSelection::All;
    }
    if (option == "--cookable"sv) {
        args.AppMutable().recipe_selection = RecipeSelection::Cookable;
    }
}

void SetValueFlag(Args& args,
                  const std::pair<std::string_view, std::string_view>&
                      option_value) noexcept {
    const auto& [option, value] = option_value;

    if (option == "--json-out"sv) {
        args.OutMutable().json_out_path = fs::path{value};
    } else if (option == "--yaml-out"sv) {
        args.OutMutable().yaml_out_path = fs::path{value};
    } else if (option == "--db-path"sv) {
        args.AppMutable().db_path = fs::path{value};
    }
}

}  // namespace

namespace io::arg {

bool Args::ShowHelp() const noexcept { return show_help_; }

void Args::SetHelp(bool help_flag) noexcept { show_help_ = help_flag; }

const Args::AppArgs& Args::App() const noexcept { return app_; }

Args::AppArgs& Args::AppMutable() noexcept { return app_; }

const Args::ArgsOut& Args::Out() const noexcept { return out_; }

Args::ArgsOut& Args::OutMutable() noexcept { return out_; }

bool HasAnyOutput(const Args::ArgsOut& args) noexcept {
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
        if (!IsValidToken(token)) {
            return std::unexpected(
                "Unknown argument: "s.append(std::string(token)));
        }

        const auto [option, value] = SplitOption(token);
        const auto type = GetFlagType(option);
        if (!type.has_value()) {
            return std::unexpected(
                "Unknown type for argument: "s.append(std::string(option)));
        }

        switch (*type) {
            case FlagInfo::Type::Value:
                SetValueFlag(parsed, std::make_pair(option, value));
                break;
            case FlagInfo::Type::Bool:
                SetBoolFlag(parsed, option);
                break;
            case FlagInfo::Type::Command:
                break;
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

}  // namespace io::arg
