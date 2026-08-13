#pragma once

#include "types/kitchen/Types.hpp"

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifndef RECIPES_PROJECT_ROOT
#define RECIPES_PROJECT_ROOT "."
#endif

namespace io::arg {

[[nodiscard]] inline std::filesystem::path GetProjectDataDir() {
    return std::filesystem::path{RECIPES_PROJECT_ROOT} / "data";
}

[[nodiscard]] inline std::filesystem::path GetProjectOutDir() {
    return std::filesystem::path{RECIPES_PROJECT_ROOT} / "out";
}

/**
 * @brief Parsed command-line arguments for the application.
 */
class Args final {
 public:
    struct AppArgs final {
        types::RecipeSelection recipe_selection{types::RecipeSelection::All};
        std::filesystem::path db_path{GetProjectDataDir() / "table.db"};
        std::vector<types::Product> products;
        std::vector<types::Recipe> recipes;
    };
    struct ArgsOut final {
        bool is_full_info{true};
        bool write_console{true};
        bool write_json{true};
        bool write_yaml{true};
        std::filesystem::path json_out_path{GetProjectOutDir() / "info.json"};
        std::filesystem::path yaml_out_path{GetProjectOutDir() / "info.yaml"};
    };

    /**
     * @brief Returns whether help output should be shown.
     * @return `true` if help output is requested.
     */
    [[nodiscard]] bool ShowHelp() const noexcept;

    /*
     * @brief Set `show_help` on `help_flag`
     * @param help_flag bool flag to show or not help
     */
    void SetHelp(bool help_flag) noexcept;

    /**
     * @brief Returns application input arguments.
     * @return Immutable application arguments.
     */
    [[nodiscard]] const AppArgs& App() const noexcept;

    /**
     * @brief Returns mutable application input arguments.
     * @return Mutable application arguments for controlled setup code.
     */
    [[nodiscard]] AppArgs& AppMutable() noexcept;

    /**
     * @brief Returns report output arguments.
     * @return Immutable output arguments.
     */
    [[nodiscard]] const ArgsOut& Out() const noexcept;

    /**
     * @brief Returns mutable report output arguments.
     * @return Mutable output arguments for controlled setup code.
     */
    [[nodiscard]] ArgsOut& OutMutable() noexcept;

 private:
    friend std::expected<Args, std::string> ParseArgs(
        std::span<const std::string_view> argv);

    bool show_help_{false};
    AppArgs app_{};
    ArgsOut out_{};
};

/**
 * @brief Checks whether at least one report output is enabled.
 * @param args Output arguments.
 * @return `true` if any output target is enabled.
 */
[[nodiscard]] bool HasAnyOutput(const Args::ArgsOut& args) noexcept;

/**
 * @brief Parses CLI arguments into strongly typed options.
 * @param argv Full argument list, including program name at index 0.
 * @return Parsed arguments or human-readable parse error.
 */
[[nodiscard]] std::expected<Args, std::string> ParseArgs(
    std::span<const std::string_view> argv);

/**
 * @brief Builds CLI usage text for the program.
 * @param program_name Executable name to show in usage examples.
 * @return Fully formatted usage text.
 */
[[nodiscard]] std::string BuildUsage(std::string_view program_name);

}  // namespace io::arg
