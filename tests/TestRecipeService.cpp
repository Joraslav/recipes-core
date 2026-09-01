#include "gtest/gtest.h"

#include "app/Error.hpp"
#include "app/RecipeRepository.hpp"
#include "app/RecipeService.hpp"
#include "types/kitchen/Types.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>
#include <vector>

using app::RecipeRepository;
using app::RecipeService;
using types::Dimension;
using types::Product;
using types::Recipe;

namespace {

class FakeRecipeRepository final : public RecipeRepository {
 public:
    std::expected<Product, app::Error> CreateProduct(
        const Product& product) override {
        created_product_ = product;
        created_product_.SetId(kProductId);
        return created_product_;
    }

    std::expected<std::optional<Product>, app::Error> GetProduct(
        int64_t) override {  // NOLINT (readability-named-parameter)
        return std::optional<Product>{created_product_};
    }

    std::expected<std::vector<Product>, app::Error> GetProducts() override {
        return std::vector<Product>{created_product_};
    }

    std::expected<bool, app::Error> UpdateProduct(
        int64_t,  // NOLINT (readability-named-parameter)
        const Product&) override {
        return update_result_;
    }

    std::expected<bool, app::Error> DeleteProduct(
        int64_t) override {  // NOLINT (readability-named-parameter)
        return delete_result_;
    }

    std::expected<Recipe, app::Error> CreateRecipe(
        const Recipe& recipe) override {
        return recipe;
    }

    std::expected<std::optional<Recipe>, app::Error> GetRecipe(
        int64_t) override {  // NOLINT (readability-named-parameter)
        return std::optional<Recipe>{};
    }

    std::expected<std::vector<Recipe>, app::Error> GetRecipes() override {
        return std::vector<Recipe>{};
    }

    std::expected<std::vector<Recipe>, app::Error> GetCookableRecipes()
        override {
        return std::vector<Recipe>{};
    }

    std::expected<bool, app::Error> UpdateRecipe(
        int64_t,  // NOLINT (readability-named-parameter)
        const Recipe&) override {
        return update_result_;
    }

    std::expected<bool, app::Error> DeleteRecipe(
        int64_t) override {  // NOLINT (readability-named-parameter)
        return delete_result_;
    }

    void SetUpdateResult(bool result) noexcept { update_result_ = result; }

 private:
    static constexpr int64_t kProductId = 1;

    Product created_product_{"", 0, Dimension::Piece};
    bool update_result_{true};
    bool delete_result_{true};
};

class TestRecipeService : public ::testing::Test {
 protected:
    [[nodiscard]] static Product MakeProduct(std::string_view name,
                                             int amount) {
        return Product{name, amount, Dimension::Piece};
    }

    FakeRecipeRepository repository;
    RecipeService service{repository};
};

TEST_F(TestRecipeService, CreateProduct_ValidProduct_ReturnsPersistedProduct) {
    const auto result = service.CreateProduct(MakeProduct("Egg", 4));

    ASSERT_TRUE(result.has_value()) << result.error().GetMessage();
    ASSERT_TRUE(result->GetId().has_value());
    EXPECT_EQ(result->GetId().value(), 1);
    EXPECT_EQ(result->GetName(), "Egg");
}

TEST_F(TestRecipeService, CreateProduct_EmptyName_ReturnsValidationError) {
    const auto result = service.CreateProduct(MakeProduct("", 4));

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().GetCode(), app::ErrorCode::Validation);
}

TEST_F(TestRecipeService,
       CreateRecipe_NegativeIngredientAmount_ReturnsValidationError) {
    const Recipe recipe{"Omelet", {MakeProduct("Egg", -1)}};

    const auto result = service.CreateRecipe(recipe);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().GetCode(), app::ErrorCode::Validation);
}

TEST_F(TestRecipeService, UpdateProduct_MissingProduct_ReturnsNotFoundError) {
    repository.SetUpdateResult(false);

    const auto result = service.UpdateProduct(99, MakeProduct("Egg", 4));

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().GetCode(), app::ErrorCode::NotFound);
}

}  // namespace