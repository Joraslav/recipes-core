#include "gtest/gtest.h"

#include "types/kitchen/Types.hpp"

#include <chrono>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using types::Dates;
using types::Dimension;
using types::Product;
using types::Recipe;

class TestKitchenTypes : public ::testing::Test {
 protected:
    static constexpr std::string_view kProductName = "Apple";
    static constexpr int kDefaultAmount = 3;
};

TEST_F(TestKitchenTypes, Product_ConstructedWithDefaults_StoresInitialState) {
    const Product product(kProductName, kDefaultAmount, Dimension::PIECE);

    EXPECT_EQ(product.GetName(), kProductName);
    EXPECT_EQ(product.GetAmount(), kDefaultAmount);
    EXPECT_EQ(product.GetDimension(), Dimension::PIECE);
    EXPECT_FALSE(product.GetManufactureDate().has_value());
    EXPECT_FALSE(product.GetExpirationDate().has_value());
}

TEST_F(TestKitchenTypes, Product_Setters_UpdateFields) {
    Product product("Placeholder", 1, Dimension::GRAMM);
    const auto manufacture_date = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{2} / std::chrono::day{1}};
    const auto expiration_date = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{2} / std::chrono::day{10}};

    product.SetName("Orange");
    product.SetAmount(12);
    product.SetManufactureDate(manufacture_date);
    product.SetExpirationDate(expiration_date);

    EXPECT_EQ(product.GetName(), "Orange");
    EXPECT_EQ(product.GetAmount(), 12);
    ASSERT_TRUE(product.GetManufactureDate().has_value());
    ASSERT_TRUE(product.GetExpirationDate().has_value());
    EXPECT_EQ(product.GetManufactureDate().value(), manufacture_date);
    EXPECT_EQ(product.GetExpirationDate().value(), expiration_date);
}

TEST_F(TestKitchenTypes, Product_IsFresh_NoExpirationDate_ReturnsTrue) {
    const Product product("Salt", 100, Dimension::GRAMM);
    EXPECT_TRUE(product.IsFresh());
}

TEST_F(TestKitchenTypes, Product_IsFresh_ExpirationInFuture_ReturnsTrue) {
    const auto tomorrow = std::chrono::floor<std::chrono::days>(
                              std::chrono::system_clock::now()) +
                          std::chrono::days{1};
    const Product product(
        "Milk", 1, Dimension::LITER,
        Dates{.manufacture = std::nullopt, .expiration = tomorrow});
    EXPECT_TRUE(product.IsFresh());
}

TEST_F(TestKitchenTypes, Product_IsFresh_ExpirationInPast_ReturnsFalse) {
    const auto yesterday = std::chrono::floor<std::chrono::days>(
                               std::chrono::system_clock::now()) -
                           std::chrono::days{1};
    const Product product(
        "Milk", 1, Dimension::LITER,
        Dates{.manufacture = std::nullopt, .expiration = yesterday});
    EXPECT_FALSE(product.IsFresh());
}

TEST_F(TestKitchenTypes,
       Recipe_ConstructedWithIngredients_StoresAllIngredients) {
    const Recipe recipe("Salad", {Product("Tomato", 2, Dimension::PIECE),
                                  Product("Oil", 20, Dimension::MILLILITER)});

    EXPECT_EQ(recipe.GetName(), "Salad");
    ASSERT_EQ(recipe.GetIngredients().size(), 2U);
    EXPECT_EQ(recipe.GetIngredients()[0].GetName(), "Tomato");
    EXPECT_EQ(recipe.GetIngredients()[1].GetName(), "Oil");
}

TEST_F(TestKitchenTypes, Recipe_AddIngredient_AppendsSingleIngredient) {
    Recipe recipe("Soup");

    recipe.AddIngredient(Product("Water", 500, Dimension::MILLILITER));

    ASSERT_EQ(recipe.GetIngredients().size(), 1U);
    EXPECT_EQ(recipe.GetIngredients().front().GetName(), "Water");
    EXPECT_EQ(recipe.GetIngredients().front().GetAmount(), 500);
}

TEST_F(TestKitchenTypes,
       Recipe_AddIngredientsInitializerList_AppendsAllIngredients) {
    Recipe recipe("Porridge");
    recipe.AddIngredient(Product("Milk", 200, Dimension::MILLILITER));

    recipe.AddIngredients({
        Product("Oat", 100, Dimension::GRAMM),
        Product("Honey", 20, Dimension::GRAMM),
    });

    ASSERT_EQ(recipe.GetIngredients().size(), 3U);
    EXPECT_EQ(recipe.GetIngredients()[0].GetName(), "Milk");
    EXPECT_EQ(recipe.GetIngredients()[1].GetName(), "Oat");
    EXPECT_EQ(recipe.GetIngredients()[2].GetName(), "Honey");
}

TEST_F(TestKitchenTypes,
       Recipe_AddIngredientsVector_AppendsIngredientCollection) {
    Recipe recipe("Toast");
    recipe.AddIngredients({Product("Bread", 2, Dimension::PIECE)});

    std::vector<Product> additional_ingredients{
        Product("Bread", 1, Dimension::PIECE),
        Product("Cheese", 30, Dimension::GRAMM),
    };

    recipe.AddIngredients(std::move(additional_ingredients));

    ASSERT_EQ(recipe.GetIngredients().size(), 3U);
    EXPECT_EQ(recipe.GetIngredients()[0].GetName(), "Bread");
    EXPECT_EQ(recipe.GetIngredients()[0].GetAmount(), 2);
    EXPECT_EQ(recipe.GetIngredients()[1].GetName(), "Bread");
    EXPECT_EQ(recipe.GetIngredients()[1].GetAmount(), 1);
    EXPECT_EQ(recipe.GetIngredients()[2].GetName(), "Cheese");
}

struct DimensionCase final {
    Dimension dimension;
    std::string expected_string;
};

class TestKitchenTypesDimensionParameterized
    : public ::testing::Test,
      public ::testing::WithParamInterface<DimensionCase> {};

TEST_P(TestKitchenTypesDimensionParameterized,
       Product_GetDimensionInString_MapsEachDimensionCorrectly) {
    const DimensionCase& test_case = GetParam();
    Product product("Any", 1, test_case.dimension);
    EXPECT_EQ(product.GetDimensionInString(), test_case.expected_string);
}

INSTANTIATE_TEST_SUITE_P(
    DimensionFormatting, TestKitchenTypesDimensionParameterized,
    ::testing::Values(
        DimensionCase{.dimension = Dimension::GRAMM, .expected_string = "gr"},
        DimensionCase{.dimension = Dimension::KILOGRAMM,
                      .expected_string = "kg"},
        DimensionCase{.dimension = Dimension::MILLILITER,
                      .expected_string = "ml"},
        DimensionCase{.dimension = Dimension::LITER, .expected_string = "l"},
        DimensionCase{.dimension = Dimension::PIECE, .expected_string = "pc"}));

}  // namespace
