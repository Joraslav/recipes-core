#include "IO.hpp"

#include "args/Args.hpp"
#include "concepts/Concepts.hpp"
#include "io/console/Report.hpp"
#include "io/json/Report.hpp"
#include "io/yaml/Report.hpp"
#include "types/kitchen/Types.hpp"

#include <cstddef>
#include <expected>
#include <functional>
#include <future>
#include <iostream>
#include <ostream>
#include <span>
#include <system_error>
#include <vector>

using types::Recipe;
using namespace io::json;
using namespace io::yaml;
using namespace io::cli;
using concepts::ProductOrRecipe;
using namespace io::arg;
using ArgsOut = io::arg::Args::ArgsOut;

namespace {

constexpr size_t kMinForParallel = 10;

template <ProductOrRecipe Tv>
std::expected<void, std::error_code> ParallelWriteReports(
    std::span<const Tv> items, const ArgsOut& args, std::ostream& out) {
    std::vector<std::future<std::expected<void, std::error_code>>> futures;

    // Launch JSON write task if enabled
    if (args.write_json) {
        futures.push_back(std::async(std::launch::async, WriteItemsJson<Tv>,
                                     items, args.json_out_path));
    }

    // Launch YAML write task if enabled
    if (args.write_yaml) {
        futures.push_back(std::async(std::launch::async, WriteItemsYaml<Tv>,
                                     items, args.yaml_out_path));
    }

    // Launch console print task if enabled (always sequential, doesn't block
    // file I/O)
    std::future<void> console_future;
    if (args.write_console) {
        console_future = std::async(std::launch::async, PrintItems<Tv>, items,
                                    args.is_full_info, std::ref(out));
    }

    // Wait for all file operations and check for errors
    for (auto& future : futures) {
        auto result = future.get();
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
    }

    // Wait for console output
    if (args.write_console) {
        console_future.get();
    }

    return {};
}

}  // namespace

namespace io {

std::expected<void, std::error_code> ReportRecipes(
    std::span<const Recipe> recipes, const Args::ArgsOut& args,
    std::ostream& out) {
    if (!args.write_console && !args.write_json && !args.write_yaml) {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }

    const bool should_run_parallel =
        recipes.size() > kMinForParallel && args.write_json && args.write_yaml;
    if (should_run_parallel) {
        return ParallelWriteReports(recipes, args, out);
    }

    if (args.write_console) {
        PrintItems(recipes, args.is_full_info, out);
    }
    if (args.write_json) {
        auto json_result = WriteItemsJson(recipes, args.json_out_path);
        if (!json_result.has_value()) {
            return std::unexpected(json_result.error());
        }
    }
    if (args.write_yaml) {
        auto yaml_result = WriteItemsYaml(recipes, args.yaml_out_path);
        if (!yaml_result.has_value()) {
            return std::unexpected(yaml_result.error());
        }
    }

    return {};
}

}  // namespace io
