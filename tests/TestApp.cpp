#include "gtest/gtest.h"

#include "app/App.hpp"
#include "io/args/Args.hpp"
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
                                  Dimension dimension,
                                  Dates dates = {}) noexcept {
    return Product{name, amount, dimension, dates};
}

[[nodiscard]] Recipe MakeRecipe(std::string_view name,
                                std::initializer_list<Product> ingredients) noexcept {
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
        args.AppMutable() = std::move(app_args);
        args.OutMutable().write_json = false;
        args.OutMutable().write_yaml = false;
        return args;
    }
};

TEST_F(TestApp, Execute_AllRecipesSelection_ReturnsPersistedRecipes) {
    AppArgs app_args = MakeAppArgs();
    app_args.products = {
        MakeProduct("Egg"sv, 4, Dimension::Piece),
        MakeProduct("Milk"sv, 250, Dimension::Milliliter),
    };
    app_args.recipes = {
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Egg"sv, 2, Dimension::Piece),
                       MakeProduct("Milk"sv, 100, Dimension::Milliliter),
                   }),
        MakeRecipe(kOmelet, {MakeProduct("Egg"sv, 3, Dimension::Piece)}),
    };

    auto run_result = app::Execute(app_args);

    ASSERT_TRUE(run_result.has_value());
    ASSERT_EQ(run_result->size(), 2U);
    
    const auto has_pancake = std::ranges::any_of(
        *run_result,
        [](const Recipe& recipe) { return recipe.GetName() == kPancake; });
    const auto has_omelet = std::ranges::any_of(
        *run_result,
        [](const Recipe& recipe) { return recipe.GetName() == kOmelet; });
    
    EXPECT_TRUE(has_pancake);
    EXPECT_TRUE(has_omelet);
}

TEST_F(TestApp, Execute_CookableSelection_ReturnsOnlyCookableRecipes) {
    AppArgs app_args = MakeAppArgs();
    app_args.recipe_selection = RecipeSelection::Cookable;
    app_args.products = {
        MakeProduct("Egg"sv, 4, Dimension::Piece),
        MakeProduct("Milk"sv, 200, Dimension::Milliliter),
        MakeProduct("Flour"sv, 200, Dimension::Gramm),
    };
    app_args.recipes = {
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Egg"sv, 2, Dimension::Piece),
                       MakeProduct("Milk"sv, 100, Dimension::Milliliter),
                       MakeProduct("Flour"sv, 150, Dimension::Gramm),
                   }),
        MakeRecipe(kOmelet, {MakeProduct("Egg"sv, 5, Dimension::Piece)}),
    };

    auto run_result = app::Execute(app_args);

    ASSERT_TRUE(run_result.has_value());
    ASSERT_EQ(run_result->size(), 1U);
    EXPECT_EQ(run_result->front().GetName(), kPancake);
}

TEST_F(TestApp, Run_ConsoleOutputEnabled_UsesReportSettings) {
    AppArgs app_args = MakeAppArgs();
    app_args.recipe_selection = RecipeSelection::Cookable;
    app_args.products = {
        MakeProduct("Milk"sv, 200, Dimension::Milliliter),
        MakeProduct("Flour"sv, 150, Dimension::Gramm),
    };
    app_args.recipes = {
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Milk"sv, 200, Dimension::Milliliter),
                       MakeProduct("Flour"sv, 150, Dimension::Gramm),
                   }),
    };

    Args args = MakeArgs(std::move(app_args));
    args.OutMutable().is_full_info = false;
    args.OutMutable().write_console = true;

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
                   {MakeProduct("Water"sv, 300, Dimension::Milliliter)}),
    };

    Args args = MakeArgs(std::move(app_args));
    args.OutMutable().write_console = false;

    std::ostringstream out;
    auto run_result = app::Run(args, out);

    ASSERT_TRUE(run_result.has_value());
    EXPECT_TRUE(out.str().empty());
}

}  // namespace
