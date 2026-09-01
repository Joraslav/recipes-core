#pragma once

#include "Error.hpp"
#include "RecipeRepository.hpp"
#include "types/kitchen/Types.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace app {

/**
 * @brief Transport-independent product and recipe CRUD use cases.
 * @details The supplied repository is non-owning and must outlive this
 * service instance.
 */
class RecipeService final {
 public:
    /**
     * @brief Creates a service over a repository implementation.
     * @param repository Repository used to execute all use cases.
     * @pre `repository` outlives this service.
     */
    explicit RecipeService(RecipeRepository& repository) noexcept;

    /** @brief Validates and persists a product. */
    [[nodiscard]] std::expected<types::Product, Error> CreateProduct(
        const types::Product& product);
    /** @brief Gets a product, returning `std::nullopt` when it is absent. */
    [[nodiscard]] std::expected<std::optional<types::Product>, Error>
    GetProduct(int64_t product_id);
    /** @brief Gets all products. */
    [[nodiscard]] std::expected<std::vector<types::Product>, Error>
    GetProducts();
    /** @brief Validates and replaces a product, returning `NotFound` if absent.
     */
    [[nodiscard]] std::expected<void, Error> UpdateProduct(
        int64_t product_id, const types::Product& product);
    /** @brief Deletes a product, returning `NotFound` if absent. */
    [[nodiscard]] std::expected<void, Error> DeleteProduct(int64_t product_id);

    /** @brief Validates and persists a recipe. */
    [[nodiscard]] std::expected<types::Recipe, Error> CreateRecipe(
        const types::Recipe& recipe);
    /** @brief Gets a recipe, returning `std::nullopt` when it is absent. */
    [[nodiscard]] std::expected<std::optional<types::Recipe>, Error> GetRecipe(
        int64_t recipe_id);
    /** @brief Gets all recipes. */
    [[nodiscard]] std::expected<std::vector<types::Recipe>, Error> GetRecipes();
    /** @brief Gets recipes whose required ingredients are available. */
    [[nodiscard]] std::expected<std::vector<types::Recipe>, Error>
    GetCookableRecipes();
    /** @brief Validates and replaces a recipe, returning `NotFound` if absent.
     */
    [[nodiscard]] std::expected<void, Error> UpdateRecipe(
        int64_t recipe_id, const types::Recipe& recipe);
    /** @brief Deletes a recipe, returning `NotFound` if absent. */
    [[nodiscard]] std::expected<void, Error> DeleteRecipe(int64_t recipe_id);

 private:
    [[nodiscard]] static std::expected<void, Error> ValidateProduct(
        const types::Product& product);
    [[nodiscard]] static std::expected<void, Error> ValidateRecipe(
        const types::Recipe& recipe);

    RecipeRepository* repository_;
};

}  // namespace app