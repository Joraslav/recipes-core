#include "gtest/gtest.h"

#include "db/DBManager.hpp"
#include "types/kitchen/Types.hpp"

#include <chrono>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace {

using db::DBManager;
using types::Dates;
using types::Dimension;
using types::Product;
using types::Recipe;

Product MakeProduct(std::string_view name, int amount, Dimension dimension,
                    Dates dates = Dates{}) {
    return Product{name, amount, dimension, dates};
}

Recipe MakeRecipe(std::string_view name,
                  std::initializer_list<Product> ingredients) {
    return Recipe{name, ingredients};
}

class TestDBManager : public ::testing::Test {
 protected:
    static constexpr std::string_view kPancake = "Pancake";
    static constexpr std::string_view kOmelet = "Omelet";

    DBManager db_manager;

    static void ExpectProductEq(const Product& actual,
                                const Product& expected) {
        EXPECT_EQ(actual.GetName(), expected.GetName());
        EXPECT_EQ(actual.GetAmount(), expected.GetAmount());
        EXPECT_EQ(actual.GetDimension(), expected.GetDimension());
        EXPECT_EQ(actual.GetDates().manufacture,
                  expected.GetDates().manufacture);
        EXPECT_EQ(actual.GetDates().expiration, expected.GetDates().expiration);
    }

    static const Recipe* FindRecipeByName(const std::vector<Recipe>& recipes,
                                          std::string_view name) {
        for (const auto& recipe : recipes) {
            if (recipe.GetName() == name) {
                return &recipe;
            }
        }
        return nullptr;
    }

    static const Product* FindIngredient(const Recipe& recipe,
                                         std::string_view ingredient_name,
                                         Dimension dimension) {
        for (const auto& ingredient : recipe.GetIngredients()) {
            if (ingredient.GetName() == ingredient_name &&
                ingredient.GetDimension() == dimension) {
                return &ingredient;
            }
        }
        return nullptr;
    }
};

TEST_F(TestDBManager, InsertProduct_WithDates_PersistsAllFields) {
    const auto manufacture = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{1} / std::chrono::day{2}};
    const auto expiration = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{1} / std::chrono::day{10}};
    const Product milk = MakeProduct(
        "Milk", 1500, Dimension::Milliliter,
        Dates{.manufacture = manufacture, .expiration = expiration});

    db_manager.InsertProduct(milk);

    const std::vector<Product> products = db_manager.GetAllProducts();
    ASSERT_EQ(products.size(), 1U);
    ExpectProductEq(products.front(), milk);
}

TEST_F(TestDBManager, InsertProduct_WithoutDates_StoresNullDates) {
    const Product flour = MakeProduct("Flour", 1000, Dimension::Gramm);

    db_manager.InsertProduct(flour);

    const std::vector<Product> products = db_manager.GetAllProducts();
    ASSERT_EQ(products.size(), 1U);
    EXPECT_EQ(products.front().GetName(), "Flour");
    EXPECT_FALSE(products.front().GetManufactureDate().has_value());
    EXPECT_FALSE(products.front().GetExpirationDate().has_value());
}

TEST_F(TestDBManager, InsertProduct_OnConflict_UpdatesExistingProduct) {
    db_manager.InsertProduct(MakeProduct("Egg", 4, Dimension::Piece));
    db_manager.InsertProduct(MakeProduct("Egg", 10, Dimension::Piece));

    const std::vector<Product> products = db_manager.GetAllProducts();
    ASSERT_EQ(products.size(), 1U);
    EXPECT_EQ(products.front().GetName(), "Egg");
    EXPECT_EQ(products.front().GetAmount(), 10);
    EXPECT_EQ(products.front().GetDimension(), Dimension::Piece);
}

TEST_F(TestDBManager, InsertProducts_WithMultipleItems_ReturnsAllProducts) {
    const std::vector<Product> input_products{
        MakeProduct("Sugar", 300, Dimension::Gramm),
        MakeProduct("Water", 2, Dimension::Liter),
        MakeProduct("Salt", 1, Dimension::Kilogramm),
    };

    db_manager.InsertProducts(input_products);

    const std::vector<Product> products = db_manager.GetAllProducts();
    ASSERT_EQ(products.size(), input_products.size());

    bool has_sugar = false;
    bool has_water = false;
    bool has_salt = false;
    for (const auto& product : products) {
        if (product.GetName() == "Sugar" && product.GetAmount() == 300 &&
            product.GetDimension() == Dimension::Gramm) {
            has_sugar = true;
        }
        if (product.GetName() == "Water" && product.GetAmount() == 2 &&
            product.GetDimension() == Dimension::Liter) {
            has_water = true;
        }
        if (product.GetName() == "Salt" && product.GetAmount() == 1 &&
            product.GetDimension() == Dimension::Kilogramm) {
            has_salt = true;
        }
    }

    EXPECT_TRUE(has_sugar);
    EXPECT_TRUE(has_water);
    EXPECT_TRUE(has_salt);
}

TEST_F(TestDBManager,
       InsertRecipe_NewRecipeWithIngredients_AppearsInAllRecipes) {
    const Recipe pancake = MakeRecipe(
        kPancake, {
                      MakeProduct("Milk", 200, Dimension::Milliliter),
                      MakeProduct("Flour", 150, Dimension::Gramm),
                  });

    db_manager.InsertRecipe(pancake);

    const std::vector<Recipe> recipes = db_manager.GetAllRecipes();
    ASSERT_EQ(recipes.size(), 1U);
    EXPECT_EQ(recipes.front().GetName(), kPancake);
    ASSERT_EQ(recipes.front().GetIngredients().size(), 2U);

    const Product* milk =
        FindIngredient(recipes.front(), "Milk", Dimension::Milliliter);
    ASSERT_NE(milk, nullptr);
    EXPECT_EQ(milk->GetAmount(), 200);

    const Product* flour =
        FindIngredient(recipes.front(), "Flour", Dimension::Gramm);
    ASSERT_NE(flour, nullptr);
    EXPECT_EQ(flour->GetAmount(), 150);
}

TEST_F(TestDBManager,
       InsertRecipe_ExistingRecipe_ReplacesIngredientListForSameName) {
    db_manager.InsertRecipe(MakeRecipe(
        kPancake, {
                      MakeProduct("Egg", 1, Dimension::Piece),
                      MakeProduct("Milk", 100, Dimension::Milliliter),
                  }));
    db_manager.InsertRecipe(
        MakeRecipe(kPancake, {
                                 MakeProduct("Egg", 2, Dimension::Piece),
                             }));

    const std::vector<Recipe> recipes = db_manager.GetAllRecipes();
    ASSERT_EQ(recipes.size(), 1U);
    EXPECT_EQ(recipes.front().GetName(), kPancake);
    ASSERT_EQ(recipes.front().GetIngredients().size(), 1U);
    EXPECT_EQ(recipes.front().GetIngredients().front().GetName(), "Egg");
    EXPECT_EQ(recipes.front().GetIngredients().front().GetAmount(), 2);
}

TEST_F(TestDBManager,
       InsertRecipes_MultipleRecipesWithOverlap_PersistsAllEntities) {
    const std::vector<Recipe> input_recipes{
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Milk", 250, Dimension::Milliliter),
                       MakeProduct("Egg", 2, Dimension::Piece),
                   }),
        MakeRecipe(kOmelet,
                   {
                       MakeProduct("Egg", 3, Dimension::Piece),
                       MakeProduct("Milk", 50, Dimension::Milliliter),
                   }),
    };

    db_manager.InsertRecipes(input_recipes);

    const std::vector<Recipe> recipes = db_manager.GetAllRecipes();
    ASSERT_EQ(recipes.size(), 2U);
    EXPECT_NE(FindRecipeByName(recipes, kPancake), nullptr);
    EXPECT_NE(FindRecipeByName(recipes, kOmelet), nullptr);
}

TEST_F(TestDBManager,
       GetRecipeIngredients_UnknownRecipeId_ReturnsEmptyCollection) {
    const std::vector<Product> ingredients =
        db_manager.GetRecipeIngredients(9999);
    EXPECT_TRUE(ingredients.empty());
}

TEST_F(TestDBManager,
       GetCookableRecipes_WhenMissingProducts_ReturnsOnlyCookableRecipes) {
    const std::vector<Product> stock{
        MakeProduct("Egg", 4, Dimension::Piece),
        MakeProduct("Milk", 200, Dimension::Milliliter),
        MakeProduct("Flour", 300, Dimension::Gramm),
    };
    db_manager.InsertProducts(stock);

    const std::vector<Recipe> recipes_to_insert{
        MakeRecipe(kPancake,
                   {
                       MakeProduct("Egg", 2, Dimension::Piece),
                       MakeProduct("Milk", 100, Dimension::Milliliter),
                       MakeProduct("Flour", 150, Dimension::Gramm),
                   }),
        MakeRecipe(kOmelet,
                   {
                       MakeProduct("Egg", 5, Dimension::Piece),
                   }),
    };
    db_manager.InsertRecipes(recipes_to_insert);

    const std::vector<Recipe> cookable = db_manager.GetCookableRecipes();
    ASSERT_EQ(cookable.size(), 1U);
    EXPECT_EQ(cookable.front().GetName(), kPancake);
}

struct CookableScenario final {
    std::string name;
    std::vector<Product> stock;
    Recipe recipe;
    bool expected_cookable;
};

class TestDBManagerCookableParameterized
    : public TestDBManager,
      public ::testing::WithParamInterface<CookableScenario> {};

TEST_P(TestDBManagerCookableParameterized,
       GetCookableRecipes_VariousInventoryStates_MatchesExpectation) {
    const CookableScenario& scenario = GetParam();
    db_manager.InsertProducts(scenario.stock);
    db_manager.InsertRecipe(scenario.recipe);

    const std::vector<Recipe> cookable = db_manager.GetCookableRecipes();
    const bool is_present =
        FindRecipeByName(cookable, scenario.recipe.GetName()) != nullptr;
    EXPECT_EQ(is_present, scenario.expected_cookable);
}

INSTANTIATE_TEST_SUITE_P(
    InventoryScenarios, TestDBManagerCookableParameterized,
    ::testing::Values(
        CookableScenario{
            .name = "EnoughStockIsCookable",
            .stock = {MakeProduct("Milk", 500, Dimension::Milliliter)},
            .recipe = MakeRecipe(
                "MilkShake", {MakeProduct("Milk", 300, Dimension::Milliliter)}),
            .expected_cookable = true,
        },
        CookableScenario{
            .name = "InsufficientStockIsNotCookable",
            .stock = {MakeProduct("Milk", 200, Dimension::Milliliter)},
            .recipe = MakeRecipe(
                "MilkShake", {MakeProduct("Milk", 300, Dimension::Milliliter)}),
            .expected_cookable = false,
        },
        CookableScenario{
            .name = "MissingDimensionIsNotCookable",
            .stock = {MakeProduct("Sugar", 1, Dimension::Kilogramm)},
            .recipe = MakeRecipe("Tea",
                                 {MakeProduct("Sugar", 500, Dimension::Gramm)}),
            .expected_cookable = false,
        }),
    [](const testing::TestParamInfo<CookableScenario>& info) {
        return info.param.name;
    });

}  // namespace
