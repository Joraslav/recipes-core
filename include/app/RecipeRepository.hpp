#pragma once

#include "app/Error.hpp"
#include "types/kitchen/Types.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace app {

/**
 * @brief Persistence port used by recipe and product application use cases.
 * @details Implementations must translate storage failures into `Error` and
 * return `false` from update/delete methods when the requested ID is absent.
 */
class RecipeRepository {
 public:
    /** @brief Destroys a repository implementation polymorphically. */
    virtual ~RecipeRepository() = default;

    /** @brief Persists a product and returns it with its assigned ID. */
    virtual std::expected<types::Product, Error> CreateProduct(
        const types::Product& product) = 0;
    /** @brief Returns a product by ID, or `std::nullopt` when it is absent. */
    virtual std::expected<std::optional<types::Product>, Error> GetProduct(
        int64_t product_id) = 0;
    /** @brief Returns all persisted products. */
    virtual std::expected<std::vector<types::Product>, Error> GetProducts() = 0;
    /** @brief Replaces a product and reports whether its ID exists. */
    virtual std::expected<bool, Error> UpdateProduct(
        int64_t product_id, const types::Product& product) = 0;
    /** @brief Deletes a product and reports whether its ID exists. */
    virtual std::expected<bool, Error> DeleteProduct(int64_t product_id) = 0;

    /** @brief Persists a recipe and returns it with its assigned ID. */
    virtual std::expected<types::Recipe, Error> CreateRecipe(
        const types::Recipe& recipe) = 0;
    /** @brief Returns a recipe by ID, or `std::nullopt` when it is absent. */
    virtual std::expected<std::optional<types::Recipe>, Error> GetRecipe(
        int64_t recipe_id) = 0;
    /** @brief Returns all persisted recipes. */
    virtual std::expected<std::vector<types::Recipe>, Error> GetRecipes() = 0;
    /** @brief Returns recipes whose ingredients are currently available. */
    virtual std::expected<std::vector<types::Recipe>, Error>
    GetCookableRecipes() = 0;
    /** @brief Replaces a recipe and reports whether its ID exists. */
    virtual std::expected<bool, Error> UpdateRecipe(
        int64_t recipe_id, const types::Recipe& recipe) = 0;
    /** @brief Deletes a recipe and reports whether its ID exists. */
    virtual std::expected<bool, Error> DeleteRecipe(int64_t recipe_id) = 0;
};

}  // namespace app