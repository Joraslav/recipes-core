#pragma once

#include "concepts/Concepts.hpp"
#include "console/Report.hpp"
#include "io/args/Args.hpp"
#include "json/Report.hpp"
#include "types/kitchen/Types.hpp"
#include "yaml/Report.hpp"

#include <cstddef>
#include <expected>
#include <functional>
#include <future>
#include <iostream>
#include <ostream>
#include <span>
#include <system_error>
#include <vector>

namespace io {

/*
 * @brief Minimum number of items to report in parallel
 * @note @see benchmarks in bench/ThreadOutput.cpp for performance comparison
 */
constexpr size_t kMinSizeForParallelReporting = 250;

/**
 * @brief Writes a recipe list to the enabled output targets.
 * @param recipes Recipes to report.
 * @param args Output settings.
 * @param out Output stream for console output.
 * @return Error code if any write fails.
 */
[[nodiscard]] std::expected<void, std::error_code> ReportRecipes(
    std::span<const types::Recipe> recipes, const arg::Args::ArgsOut& args,
    std::ostream& out = std::cout);

/**
 * @brief Writes a product list to the enabled output targets.
 * @param products Products to report.
 * @param args Output settings.
 * @param out Output stream for console output.
 * @return Error code if any write fails.
 */
[[nodiscard]] std::expected<void, std::error_code> ReportProducts(
    std::span<const types::Product> products, const arg::Args::ArgsOut& args,
    std::ostream& out = std::cout);

/**
 * @brief Dispatches reporting to the product or recipe implementation.
 * @tparam Tv Product or recipe type.
 * @param items Items to report.
 * @param args Output settings.
 * @param out Output stream for console output.
 * @return Error code if any write fails.
 */
template <concepts::ProductOrRecipe Tv>
[[nodiscard]] std::expected<void, std::error_code> ReportsItems(
    std::span<const Tv> items, const arg::Args::ArgsOut& args,
    std::ostream& out = std::cout) {
    if constexpr (std::is_same_v<Tv, types::Product>) {
        return ReportProducts(items, args, out);
    } else if constexpr (std::is_same_v<Tv, types::Recipe>) {
        return ReportRecipes(items, args, out);
    } else {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }
}

/**
 * @brief Writes JSON/YAML in parallel when both outputs are enabled.
 * @tparam Tv Product or recipe type.
 * @param items Items to write.
 * @param args Output settings.
 * @param out Output stream for console output.
 * @return Error code if any async write fails.
 */
template <concepts::ProductOrRecipe Tv>
std::expected<void, std::error_code> ParallelReportsItems(
    std::span<const Tv> items, const arg::Args::ArgsOut& args,
    std::ostream& out) {
    using TypeFutureFile = std::expected<void, std::error_code>;
    std::vector<std::future<TypeFutureFile>> futures_files;

    // Launch console print task if enabled (always sequential, doesn't block
    // file I/O)
    std::future<void> console_future;
    if (args.write_console) {
        console_future = std::async(std::launch::async, cli::PrintItems<Tv>,
                                    items, args.is_full_info, std::ref(out));
    }

    // Launch JSON write task if enabled
    futures_files.push_back(std::async(std::launch::async,
                                       json::WriteItemsJson<Tv>, items,
                                       args.json_out_path));
    // Launch YAML write task if enabled
    futures_files.push_back(std::async(std::launch::async,
                                       yaml::WriteItemsYaml<Tv>, items,
                                       args.yaml_out_path));

    // Wait for console output
    if (args.write_console) {
        console_future.get();
    }

    // Wait for all file operations and check for errors
    for (auto& future : futures_files) {
        auto result = future.get();
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
    }

    return {};
}

}  // namespace io
