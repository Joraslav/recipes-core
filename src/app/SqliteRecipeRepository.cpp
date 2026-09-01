#include "SqliteRecipeRepository.hpp"

#include "db/DBManager.hpp"
#include "Error.hpp"
#include "types/kitchen/Types.hpp"

using app::Error;
using app::ErrorCode;
using db::DBManager;
using types::Product;
using types::Recipe;

#include <cstdint>
#include <exception>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace {

[[nodiscard]] Error StorageError(const std::exception& exception) {
    return {ErrorCode::Storage, exception.what()};
}

}  // namespace

namespace app {

SqliteRecipeRepository::SqliteRecipeRepository(DBManager& db_manager) noexcept
    : db_manager_(&db_manager) {}

std::expected<Product, Error> SqliteRecipeRepository::CreateProduct(
    const Product& product) {
    try {
        const int64_t product_id = db_manager_->CreateProduct(product);
        const auto created_product = db_manager_->GetProduct(product_id);
        if (!created_product.has_value()) {
            return std::unexpected(
                Error{ErrorCode::Storage, "Created product was not found"});
        }
        return created_product.value();
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<std::optional<Product>, Error> SqliteRecipeRepository::GetProduct(
    int64_t product_id) {
    try {
        return db_manager_->GetProduct(product_id);
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<std::vector<Product>, Error>
SqliteRecipeRepository::GetProducts() {
    try {
        return db_manager_->GetAllProducts();
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<bool, Error> SqliteRecipeRepository::UpdateProduct(
    int64_t product_id, const Product& product) {
    try {
        return db_manager_->UpdateProduct(product_id, product);
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<bool, Error> SqliteRecipeRepository::DeleteProduct(
    int64_t product_id) {
    try {
        return db_manager_->DeleteProduct(product_id);
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<Recipe, Error> SqliteRecipeRepository::CreateRecipe(
    const Recipe& recipe) {
    try {
        const int64_t recipe_id = db_manager_->CreateRecipe(recipe);
        const auto created_recipe = db_manager_->GetRecipe(recipe_id);
        if (!created_recipe.has_value()) {
            return std::unexpected(
                Error{ErrorCode::Storage, "Created recipe was not found"});
        }
        return created_recipe.value();
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<std::optional<Recipe>, Error> SqliteRecipeRepository::GetRecipe(
    int64_t recipe_id) {
    try {
        return db_manager_->GetRecipe(recipe_id);
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<std::vector<Recipe>, Error> SqliteRecipeRepository::GetRecipes() {
    try {
        return db_manager_->GetAllRecipes();
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<std::vector<Recipe>, Error>
SqliteRecipeRepository::GetCookableRecipes() {
    try {
        return db_manager_->GetCookableRecipes();
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<bool, Error> SqliteRecipeRepository::UpdateRecipe(
    int64_t recipe_id, const Recipe& recipe) {
    try {
        return db_manager_->UpdateRecipe(recipe_id, recipe);
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

std::expected<bool, Error> SqliteRecipeRepository::DeleteRecipe(
    int64_t recipe_id) {
    try {
        return db_manager_->DeleteRecipe(recipe_id);
    } catch (const std::exception& exception) {
        return std::unexpected(StorageError(exception));
    }
}

}  // namespace app