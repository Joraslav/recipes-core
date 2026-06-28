#include "Args.hpp"

#include <cstddef>
#include <expected>
#include <format>
#include <ranges>
#include <span>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

namespace {

[[nodiscard]] bool HasPrefix(std::string_view value,
                             std::string_view prefix) noexcept {
    return value.starts_with(prefix);
}

[[nodiscard]] std::expected<std::string_view, std::string> ReadOptionValue(
    std::span<const char* const> argv, size_t& index, std::string_view arg,
    const char* option_name) {
    const std::string eq_prefix = std::format("{}=", option_name);
    if (HasPrefix(arg, eq_prefix)) {
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
    const std::string_view value = argv[index];
    if (value.empty() || value.front() == '-') {
        return std::unexpected(
            std::format("Option '{}' requires a value", option_name));
    }

    return value;
}

[[nodiscard]] bool ApplyFlag(std::string_view arg, io::Args& args) noexcept {
    if (arg == "-h"sv || arg == "--help"sv) {
        args.show_help = true;
        return true;
    }

    if (arg == "--full"sv) {
        args.is_full_info = true;
        return true;
    }

    if (arg == "--short"sv) {
        args.is_full_info = false;
        return true;
    }

    if (arg == "--console"sv) {
        args.write_console = true;
        return true;
    }

    if (arg == "--no-console"sv) {
        args.write_console = false;
        return true;
    }
    //
    if (arg == "--no-json"sv) {
        args.write_json = false;
        return true;
    }

    if (arg == "--no-yaml"sv) {
        args.write_yaml = false;
        return true;
    }

    return false;
}

[[nodiscard]] std::expected<bool, std::string> TryApplyPathOption(
    std::span<const char* const> argv, size_t& index, std::string_view arg,
    io::Args& args) {
    if (arg == "--json-out"sv || HasPrefix(arg, "--json-out="sv)) {
        auto value_result = ReadOptionValue(argv, index, arg, "--json-out");
        if (!value_result.has_value()) {
            return std::unexpected(value_result.error());
        }
        args.write_json = true;
        args.json_out_path = std::string(value_result.value());
        return true;
    }

    if (arg == "--yaml-out"sv || HasPrefix(arg, "--yaml-out="sv)) {
        auto value_result = ReadOptionValue(argv, index, arg, "--yaml-out");
        if (!value_result.has_value()) {
            return std::unexpected(value_result.error());
        }
        args.write_yaml = true;
        args.yaml_out_path = std::string(value_result.value());
        return true;
    }

    return false;
}

}  // namespace

namespace io {

std::expected<Args, std::string> ParseArgs(std::span<const char* const> argv) {
    Args args{};
    std::size_t index = 1;
    bool skip_next_arg = false;

    for (const char* raw_arg : argv | std::views::drop(1)) {
        if (skip_next_arg) {
            skip_next_arg = false;
            ++index;
            continue;
        }

        const std::string_view arg = raw_arg;

        if (ApplyFlag(arg, args)) {
            ++index;
            continue;
        }

        const std::size_t current_index = index;
        auto path_option_result = TryApplyPathOption(argv, index, arg, args);
        if (!path_option_result.has_value()) {
            return std::unexpected(path_option_result.error());
        }
        if (path_option_result.value()) {
            skip_next_arg = index != current_index;
            ++index;
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
        "      --full                Print full recipe details in console\n"
        "      --short               Print only recipe names and ingredient "
        "counts\n"
        "      --console             Enable console output\n"
        "      --no-console          Disable console output\n"
        "      --json-out <path>     Write JSON report to path "
        "(default: data/out.json)\n"
        "      --yaml-out <path>     Write YAML report to path "
        "(default: data/out.yaml)\n"
        "      --no-json             Disable JSON report output\n"
        "      --no-yaml             Disable YAML report output\n",
        program_name);
}

}  // namespace io
