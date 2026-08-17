#include "io/args/Args.hpp"
#include "io/IO.hpp"
#include "types/kitchen/Types.hpp"

#include <benchmark/benchmark.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace io::arg;
using types::Dates;
using types::Dimension;
using types::Product;
using types::Recipe;

namespace fs = std::filesystem;

namespace {

[[nodiscard]] std::vector<Product> BuildProducts(size_t count) {
    std::vector<Product> products;
    products.reserve(count);
    const auto manufacture_date =
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    const Dates dates{
        .manufacture = manufacture_date,
        .expiration = manufacture_date + std::chrono::days{7},
    };
    for (size_t i = 0; i < count; ++i) {
        products.emplace_back("product", static_cast<int>(i + 1),
                              Dimension::Piece, dates);
    }
    return products;
}

[[nodiscard]] std::vector<Recipe> BuildRecipes(size_t count) {
    std::vector<Recipe> recipes;
    recipes.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::vector<Product> products = BuildProducts(5);
        recipes.emplace_back("recipe", std::move(products));
    }
    return recipes;
}

[[nodiscard]] Args::ArgsOut MakeArgs(const std::string& bench_name) {
    Args::ArgsOut args{};
    args.write_console = false;
    args.write_json = true;
    args.write_yaml = true;

    const auto out_dir = fs::temp_directory_path() / "recipes";
    args.json_out_path = out_dir / (bench_name + ".json");
    args.yaml_out_path = out_dir / (bench_name + ".yaml");
    return args;
}

void BMReportsItemsRecipes(benchmark::State& state) {
    const auto recipes_count = static_cast<size_t>(state.range(0));
    const auto recipes = BuildRecipes(recipes_count);
    auto args = MakeArgs("reports_items_" + std::to_string(recipes_count));
    std::ostringstream out;

    for (auto _ : state) {
        const auto result = io::ReportsItems<Recipe>(
            std::span<const Recipe>(recipes), args, out);
        if (!result.has_value()) {
            state.SkipWithError("ReportsItems failed");
            break;
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(recipes_count));
}

void BMParallelReportsItemsRecipes(benchmark::State& state) {
    const auto recipes_count = static_cast<size_t>(state.range(0));
    const auto recipes = BuildRecipes(recipes_count);
    auto args =
        MakeArgs("parallel_reports_items_" + std::to_string(recipes_count));
    std::ostringstream out;

    for (auto _ : state) {
        const auto result = io::ParallelReportsItems<Recipe>(
            std::span<const Recipe>(recipes), args, out);
        if (!result.has_value()) {
            state.SkipWithError("ParallelReportsItems failed");
            break;
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(recipes_count));
}

BENCHMARK(BMReportsItemsRecipes)
    ->Arg(1)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(250)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(2000);

BENCHMARK(BMParallelReportsItemsRecipes)
    ->Arg(1)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(250)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(2000);

BENCHMARK_MAIN();

}  // namespace
