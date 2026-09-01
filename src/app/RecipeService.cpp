#include "RecipeService.hpp"

#include "Error.hpp"
#include "RecipeRepository.hpp"
#include "types/kitchen/Types.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

using types::Product;
using types::Recipe;

namespace app {

RecipeService::RecipeService(RecipeRepository& repository) noexcept
    : repository_(&repository) {}

std::expected<Product, Error> RecipeService::CreateProduct(
    const Product& product) {
    if (const auto validation = ValidateProduct(product); !validation) {
        return std::unexpected(validation.error());
    }
    return repository_->CreateProduct(product);
}

std::expected<std::optional<Product>, Error> RecipeService::GetProduct(
    int64_t product_id) {
    return repository_->GetProduct(product_id);
}

std::expected<std::vector<Product>, Error> RecipeService::GetProducts() {
    return repository_->GetProducts();
}

std::expected<void, Error> RecipeService::UpdateProduct(
    int64_t product_id, const Product& product) {
    if (const auto validation = ValidateProduct(product); !validation) {
        return std::unexpected(validation.error());
    }
    const auto updated = repository_->UpdateProduct(product_id, product);
    if (!updated) {
        return std::unexpected(updated.error());
    }
    if (!updated.value()) {
        return std::unexpected(
            Error{ErrorCode::NotFound, "Product was not found"});
    }
    return {};
}

std::expected<void, Error> RecipeService::DeleteProduct(int64_t product_id) {
    const auto deleted = repository_->DeleteProduct(product_id);
    if (!deleted) {
        return std::unexpected(deleted.error());
    }
    if (!deleted.value()) {
        return std::unexpected(
            Error{ErrorCode::NotFound, "Product was not found"});
    }
    return {};
}

std::expected<Recipe, Error> RecipeService::CreateRecipe(const Recipe& recipe) {
    if (const auto validation = ValidateRecipe(recipe); !validation) {
        return std::unexpected(validation.error());
    }
    return repository_->CreateRecipe(recipe);
}

std::expected<std::optional<Recipe>, Error> RecipeService::GetRecipe(
    int64_t recipe_id) {
    return repository_->GetRecipe(recipe_id);
}

std::expected<std::vector<Recipe>, Error> RecipeService::GetRecipes() {
    return repository_->GetRecipes();
}

std::expected<std::vector<Recipe>, Error> RecipeService::GetCookableRecipes() {
    return repository_->GetCookableRecipes();
}

std::expected<void, Error> RecipeService::UpdateRecipe(int64_t recipe_id,
                                                       const Recipe& recipe) {
    if (const auto validation = ValidateRecipe(recipe); !validation) {
        return std::unexpected(validation.error());
    }
    const auto updated = repository_->UpdateRecipe(recipe_id, recipe);
    if (!updated) {
        return std::unexpected(updated.error());
    }
    if (!updated.value()) {
        return std::unexpected(
            Error{ErrorCode::NotFound, "Recipe was not found"});
    }
    return {};
}

std::expected<void, Error> RecipeService::DeleteRecipe(int64_t recipe_id) {
    const auto deleted = repository_->DeleteRecipe(recipe_id);
    if (!deleted) {
        return std::unexpected(deleted.error());
    }
    if (!deleted.value()) {
        return std::unexpected(
            Error{ErrorCode::NotFound, "Recipe was not found"});
    }
    return {};
}

std::expected<void, Error> RecipeService::ValidateProduct(
    const types::Product& product) {
    if (product.GetName().empty()) {
        return std::unexpected(
            Error{ErrorCode::Validation, "Product name must not be empty"});
    }
    if (product.GetAmount() < 0) {
        return std::unexpected(Error{ErrorCode::Validation,
                                     "Product amount must not be negative"});
    }
    return {};
}

std::expected<void, Error> RecipeService::ValidateRecipe(const Recipe& recipe) {
    if (recipe.GetName().empty()) {
        return std::unexpected(
            Error{ErrorCode::Validation, "Recipe name must not be empty"});
    }
    for (const auto& ingredient : recipe.GetIngredients()) {
        if (const auto validation = ValidateProduct(ingredient); !validation) {
            return std::unexpected(validation.error());
        }
    }
    return {};
}

}  // namespace app