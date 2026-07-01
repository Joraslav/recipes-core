#include "gtest/gtest.h"

#include "app/App.hpp"
#include "io/Args.hpp"
#include "types/kitchen/Types.hpp"

#include <algorithm>
#include <initializer_list>
#include <sstream>
#include <string_view>
#include <utility>

namespace {

using io::AppArgs;
using io::Args;
using io::RecipeSelection;
using types::Dates;
using types::Dimension;
using types::Product;
using types::Recipe;
using namespace std::string_view_literals;

[[nodiscard]] Product MakeProduct(std::string_view name, int amount,
                                  Dimension dimension, Dates dates = {}) {
    return Product{name, amount, dimension, dates};
}

[[nodiscard]] Recipe MakeRecipe(std::string_view name,
                                std::initializer_list<Product> ingredients) {
    return Recipe{name, ingredients};
}

class TestApp : public ::testing::Test {
 protected:
    static constexpr std::string_view kPancake = "Pancake"sv;
    static constexpr std::string_view kOmelet = "Omelet"sv;

    [[nodiscard]] static AppArgs MakeAppArgs() {
        AppArgs args;
        args.db_path.clear();
        return args;
    }

    [[nodiscard]] static Args MakeArgs(AppArgs app_args) {
        Args args{};
        args.app = std::move(app_args);
        args.out.write_json = false;
        args.out.write_yaml = false;
        return args;
    }
};

TEST_F(TestApp, Execute_AllRecipesSelection_ReturnsPersistedRecipes) {
    AppArgs app_args = MakeAppArgs();
    app_args.products = {
        MakeProduct("Egg"sv, 4, Dimension::PIECE),
        MakeProduct("Milk"sv, 250, Dimension::MILLILITER),
    };
    app_args.recipes = {
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Egg"sv, 2, Dimension::PIECE),
                       MakeProduct("Milk"sv, 100, Dimension::MILLILITER),
                   }),
        MakeRecipe(kOmelet, {MakeProduct("Egg"sv, 3, Dimension::PIECE)}),
    };

    auto run_result = app::Execute(app_args);

    ASSERT_TRUE(run_result.has_value());
    ASSERT_EQ(run_result->size(), 2U);
    const auto pancake_name = kPancake;
    const auto omelet_name = kOmelet;
    const auto has_pancake =
        std::ranges::any_of(*run_result, [pancake_name](const Recipe& recipe) {
            return recipe.GetName() == pancake_name;
        });
    const auto has_omelet =
        std::ranges::any_of(*run_result, [omelet_name](const Recipe& recipe) {
            return recipe.GetName() == omelet_name;
        });
    EXPECT_TRUE(has_pancake);
    EXPECT_TRUE(has_omelet);
}

TEST_F(TestApp, Execute_CookableSelection_ReturnsOnlyCookableRecipes) {
    AppArgs app_args = MakeAppArgs();
    app_args.recipe_selection = RecipeSelection::COOKABLE;
    app_args.products = {
        MakeProduct("Egg"sv, 4, Dimension::PIECE),
        MakeProduct("Milk"sv, 200, Dimension::MILLILITER),
        MakeProduct("Flour"sv, 200, Dimension::GRAMM),
    };
    app_args.recipes = {
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Egg"sv, 2, Dimension::PIECE),
                       MakeProduct("Milk"sv, 100, Dimension::MILLILITER),
                       MakeProduct("Flour"sv, 150, Dimension::GRAMM),
                   }),
        MakeRecipe(kOmelet, {MakeProduct("Egg"sv, 5, Dimension::PIECE)}),
    };

    auto run_result = app::Execute(app_args);

    ASSERT_TRUE(run_result.has_value());
    ASSERT_EQ(run_result->size(), 1U);
    EXPECT_EQ(run_result->front().GetName(), kPancake);
}

TEST_F(TestApp, Run_ConsoleOutputEnabled_UsesReportSettings) {
    AppArgs app_args = MakeAppArgs();
    app_args.recipe_selection = RecipeSelection::COOKABLE;
    app_args.products = {
        MakeProduct("Milk"sv, 200, Dimension::MILLILITER),
        MakeProduct("Flour"sv, 150, Dimension::GRAMM),
    };
    app_args.recipes = {
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Milk"sv, 200, Dimension::MILLILITER),
                       MakeProduct("Flour"sv, 150, Dimension::GRAMM),
                   }),
    };

    Args args = MakeArgs(std::move(app_args));
    args.out.is_full_info = false;
    args.out.write_console = true;

    std::ostringstream out;
    auto run_result = app::Run(args, out);

    ASSERT_TRUE(run_result.has_value());
    EXPECT_NE(out.str().find("Name: Pancake"sv), std::string_view::npos);
    EXPECT_NE(out.str().find("Ingredients count: 2"sv), std::string_view::npos);
}

TEST_F(TestApp, Run_AllOutputsDisabled_CompletesWithoutPrinting) {
    AppArgs app_args = MakeAppArgs();
    app_args.recipes = {
        MakeRecipe("Tea"sv,
                   {MakeProduct("Water"sv, 300, Dimension::MILLILITER)}),
    };

    Args args = MakeArgs(std::move(app_args));
    args.out.write_console = false;

    std::ostringstream out;
    auto run_result = app::Run(args, out);

    ASSERT_TRUE(run_result.has_value());
    EXPECT_TRUE(out.str().empty());
}

}  // namespace
