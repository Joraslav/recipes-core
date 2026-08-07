#include "gtest/gtest.h"

#include "io/console/Report.hpp"
#include "types/kitchen/Types.hpp"

#include <chrono>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace io::cli;
using types::Dates;
using types::Dimension;
using types::Product;
using types::Recipe;

class TestIOConsole : public ::testing::Test {
 protected:
    static Product MakeProduct(std::string_view name, int amount,
                               Dimension dimension, Dates dates = {}) {
        return Product{name, amount, dimension, dates};
    }

    static Recipe MakeRecipe(std::string_view name,
                             std::initializer_list<Product> ingredients) {
        return Recipe{name, ingredients};
    }
};

TEST_F(TestIOConsole, PrintProducts_EmptyInput_PrintsNoProductsMessage) {
    const std::vector<Product> products{};
    std::ostringstream out;

    PrintProducts(products, out);

    EXPECT_EQ(out.str(), "No products to display\n");
}

TEST_F(TestIOConsole, PrintProducts_OneProduct_PrintsNameAmountAndDimension) {
    const std::vector<Product> products{
        MakeProduct("Milk", 2, Dimension::Liter),
    };
    std::ostringstream out;

    PrintProducts(products, out);

    const std::string text = out.str();
    EXPECT_NE(text.find("Name: Milk\n"), std::string::npos);
    EXPECT_NE(text.find("Amount: 2l\n"), std::string::npos);
    EXPECT_NE(text.find("Manufacture: not set\n"), std::string::npos);
    EXPECT_NE(text.find("Expiration: not set\n"), std::string::npos);
    EXPECT_NE(text.find("------------------------\n"), std::string::npos);
}

TEST_F(TestIOConsole, PrintRecipes_ShortMode_PrintsOnlyIngredientsCount) {
    const std::vector<Recipe> recipes{
        MakeRecipe("Pancake",
                   {
                       MakeProduct("Milk", 200, Dimension::Milliliter),
                       MakeProduct("Flour", 150, Dimension::Gramm),
                   }),
    };
    std::ostringstream out;

    PrintRecipes(recipes, false, out);

    const std::string text = out.str();
    EXPECT_NE(text.find("Name: Pancake\n"), std::string::npos);
    EXPECT_NE(text.find("Ingredients count: 2\n"), std::string::npos);
    EXPECT_EQ(text.find("Ingredients:\n"), std::string::npos);
}

TEST_F(TestIOConsole, PrintProducts_WithDates_PrintsFormattedDates) {
    const auto manufacture = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{1} / std::chrono::day{2}};
    const auto expiration =
        std::chrono::sys_days{std::chrono::year{2099} / std::chrono::month{12} /
                              std::chrono::day{31}};
    const std::vector<Product> products{
        MakeProduct(
            "Yogurt", 1, Dimension::Piece,
            Dates{.manufacture = manufacture, .expiration = expiration}),
    };
    std::ostringstream out;

    PrintProducts(products, out);

    const std::string text = out.str();
    EXPECT_NE(text.find("Manufacture: 02-01-2026\n"), std::string::npos);
    EXPECT_NE(text.find("Expiration: 31-12-2099\n"), std::string::npos);
    EXPECT_NE(text.find("Fresh: Yes\n"), std::string::npos);
}

TEST_F(TestIOConsole, PrintProducts_ExpiredDate_PrintsFreshNo) {
    const auto expiration = std::chrono::sys_days{
        std::chrono::year{2000} / std::chrono::month{1} / std::chrono::day{1}};
    const std::vector<Product> products{
        MakeProduct(
            "Milk", 1, Dimension::Liter,
            Dates{.manufacture = std::nullopt, .expiration = expiration}),
    };
    std::ostringstream out;

    PrintProducts(products, out);

    const std::string text = out.str();
    EXPECT_NE(text.find("Expiration: 01-01-2000\n"), std::string::npos);
    EXPECT_NE(text.find("Fresh: No\n"), std::string::npos);
}

}  // namespace
