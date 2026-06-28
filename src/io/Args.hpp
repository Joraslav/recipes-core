#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace io {

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
