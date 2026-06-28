#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#ifndef RECIPES_PROJECT_ROOT
#define RECIPES_PROJECT_ROOT "."
#endif

namespace io {

[[nodiscard]] inline std::filesystem::path GetProjectOutDir() {
    return std::filesystem::path{RECIPES_PROJECT_ROOT} / "out";
}

struct ArgsOut final {
    bool show_help{false};
    bool is_full_info{false};
    bool write_console{false};
    bool write_json{false};
    bool write_yaml{false};
    std::filesystem::path json_out_path{GetProjectOutDir() / "info.json"};
    std::filesystem::path yaml_out_path{GetProjectOutDir() / "info.yaml"};
};

struct Args final {
    bool show_help{false};
    bool is_full_info{true};
    bool write_console{true};
    bool write_json{true};
    bool write_yaml{true};
    std::filesystem::path json_out_path{"data/out.json"};
    std::filesystem::path yaml_out_path{"data/out.yaml"};
};

[[nodiscard]] std::expected<Args, std::string> ParseArgs(
    std::span<const char* const> argv);

[[nodiscard]] std::string BuildUsage(std::string_view program_name);

}  // namespace io
