#include "io/Args.hpp"
#include "io/IO.hpp"
#include "types/kitchen/Types.hpp"

#include <initializer_list>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

using namespace io;
using namespace types;

namespace {
Product MakeProduct(std::string_view name, int amount, Dimension dimension,
                    Dates dates = {}) {
    return Product{name, amount, dimension, dates};
}

Recipe MakeRecipe(std::string_view name,
                  std::initializer_list<Product> ingredients) {
    return Recipe{name, ingredients};
}
}  // namespace

int main(int argc, const char* const* argv) {
    auto args_result = ParseArgs(
        std::span<const char* const>(argv, static_cast<std::size_t>(argc)));
    if (!args_result.has_value()) {
        std::cerr << args_result.error() << '\n';
        std::cerr << BuildUsage(argv[0]);
        return 2;
    }

    const Args& args = args_result.value();
    if (args.show_help) {
        std::cout << BuildUsage(argv[0]);
        return 0;
    }

    const std::vector<Recipe> recipes{
        MakeRecipe("Pancake",
                   {
                       MakeProduct("Milk", 200, Dimension::MILLILITER),
                       MakeProduct("Flour", 150, Dimension::GRAMM),
                   }),
    };

    auto run_result = RunReports(recipes, args);
    if (!run_result.has_value()) {
        std::cerr << run_result.error() << '\n';
        return 1;
    }

    return 0;
}
