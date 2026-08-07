#pragma once

#include "types/kitchen/Types.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifndef RECIPES_PROJECT_ROOT
#define RECIPES_PROJECT_ROOT "."
#endif

namespace io {

[[nodiscard]] inline std::filesystem::path GetProjectOutDir() {
    return std::filesystem::path{RECIPES_PROJECT_ROOT} / "out";
}

enum class RecipeSelection : uint8_t { ALL, COOKABLE };

struct AppArgs final {
    std::filesystem::path db_path{GetProjectOutDir() / "data/table.db"};
    std::vector<types::Product> products;
    std::vector<types::Recipe> recipes;
    RecipeSelection recipe_selection{RecipeSelection::ALL};
};

struct ArgsOut final {
    bool is_full_info{true};
    bool write_console{true};
    bool write_json{true};
    bool write_yaml{true};
    std::filesystem::path json_out_path{GetProjectOutDir() / "info.json"};
    std::filesystem::path yaml_out_path{GetProjectOutDir() / "info.yaml"};
};

struct Args final {
    bool show_help{false};
    AppArgs app{};
    ArgsOut out{};
};

[[nodiscard]] bool HasAnyOutput(const ArgsOut& args) noexcept;

[[nodiscard]] std::expected<Args, std::string> ParseArgs(
    std::span<const char* const> argv);

[[nodiscard]] std::expected<Args, std::string> ParseArgs(
    std::span<const std::string_view> argv);

[[nodiscard]] std::string BuildUsage(std::string_view program_name);

}  // namespace io
