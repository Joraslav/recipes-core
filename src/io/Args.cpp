#include "Args.hpp"

#include <cstddef>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

using io::Args;
using io::ArgsOut;
using io::RecipeSelection;

namespace {

[[nodiscard]] std::expected<std::string_view, std::string> ReadOptionValue(
    std::span<const std::string_view> argv, size_t& index, std::string_view arg,
    const char* option_name) {
    const std::string eq_prefix = std::format("{}=", option_name);
    if (arg.starts_with(eq_prefix)) {
        const auto value = arg.substr(eq_prefix.size());
        if (value.empty()) {
            return std::unexpected(
                std::format("Option '{}' requires a value", option_name));
        }
        return value;
    }

    if (index + 1 >= argv.size()) {
        return std::unexpected(
            std::format("Option '{}' requires a value", option_name));
    }

    ++index;
    const std::string_view value = argv.subspan(index, 1).front();
    if (value.empty() || value.front() == '-') {
        return std::unexpected(
            std::format("Option '{}' requires a value", option_name));
    }

    return value;
}

[[nodiscard]] bool ApplyFlag(std::string_view arg, Args& args) noexcept {
    if (arg == "-h"sv || arg == "--help"sv) {
        args.show_help = true;
        return true;
    }

    if (arg == "--full"sv) {
        args.out.is_full_info = true;
        return true;
    }

    if (arg == "--short"sv) {
        args.out.is_full_info = false;
        return true;
    }

    if (arg == "--console"sv) {
        args.out.write_console = true;
        return true;
    }

    if (arg == "--no-console"sv) {
        args.out.write_console = false;
        return true;
    }

    if (arg == "--no-json"sv) {
        args.out.write_json = false;
        return true;
    }

    if (arg == "--no-yaml"sv) {
        args.out.write_yaml = false;
        return true;
    }

    if (arg == "--all-recipes"sv) {
        args.app.recipe_selection = RecipeSelection::ALL;
        return true;
    }

    if (arg == "--cookable"sv) {
        args.app.recipe_selection = RecipeSelection::COOKABLE;
        return true;
    }

    return false;
}

[[nodiscard]] std::expected<bool, std::string> TryApplyPathOption(
    std::span<const std::string_view> argv, size_t& index, std::string_view arg,
    Args& args) {
    if (arg == "--json-out"sv || arg.starts_with("--json-out="sv)) {
        auto value_result = ReadOptionValue(argv, index, arg, "--json-out");
        if (!value_result.has_value()) {
            return std::unexpected(value_result.error());
        }
        args.out.write_json = true;
        args.out.json_out_path = std::string(value_result.value());
        return true;
    }

    if (arg == "--yaml-out"sv || arg.starts_with("--yaml-out="sv)) {
        auto value_result = ReadOptionValue(argv, index, arg, "--yaml-out");
        if (!value_result.has_value()) {
            return std::unexpected(value_result.error());
        }
        args.out.write_yaml = true;
        args.out.yaml_out_path = std::string(value_result.value());
        return true;
    }

    if (arg == "--db-path"sv || arg.starts_with("--db-path="sv)) {
        auto value_result = ReadOptionValue(argv, index, arg, "--db-path");
        if (!value_result.has_value()) {
            return std::unexpected(value_result.error());
        }
        args.app.db_path = std::string(value_result.value());
        return true;
    }

    return false;
}

}  // namespace

namespace io {

bool HasAnyOutput(const ArgsOut& args) noexcept {
    return args.write_console || args.write_json || args.write_yaml;
}

std::expected<Args, std::string> ParseArgs(std::span<const char* const> argv) {
    std::vector<std::string_view> in_args;
    in_args.reserve(argv.size());
    for (const char* arg : argv) {
        in_args.emplace_back(arg);
    }
    return ParseArgs(std::span<const std::string_view>(in_args));
}

std::expected<Args, std::string> ParseArgs(
    std::span<const std::string_view> argv) {
    Args args{};
    for (std::size_t index = 1; index < argv.size(); ++index) {
        const std::string_view arg = argv.subspan(index, 1).front();

        if (ApplyFlag(arg, args)) {
            continue;
        }

        auto path_option_result = TryApplyPathOption(argv, index, arg, args);
        if (!path_option_result.has_value()) {
            return std::unexpected(path_option_result.error());
        }
        if (path_option_result.value()) {
            continue;
        }

        return std::unexpected(std::format("Unknown argument: {}", arg));
    }

    return args;
}

std::string BuildUsage(std::string_view program_name) {
    return std::format(
        "Usage: {} [options]\n"
        "Options:\n"
        "  -h, --help                Show this help\n"
        "      --db-path <path>      Use SQLite database at path\n"
        "      --all-recipes         Query all recipes\n"
        "      --cookable            Query only cookable recipes\n"
        "      --full                Print full recipe details in console\n"
        "      --short               Print only recipe names and ingredient "
        "counts\n"
        "      --console             Enable console output\n"
        "      --no-console          Disable console output\n"
        "      --json-out <path>     Write JSON report to path "
        "(default: out/info.json)\n"
        "      --yaml-out <path>     Write YAML report to path "
        "(default: out/info.yaml)\n"
        "      --no-json             Disable JSON report output\n"
        "      --no-yaml             Disable YAML report output\n",
        program_name);
}

}  // namespace io
