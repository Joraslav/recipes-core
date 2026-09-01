#pragma once

#include "db/DBManager.hpp"
#include "Error.hpp"
#include "RecipeRepository.hpp"
#include "types/kitchen/Types.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace app {

/**
 * @brief SQLite-backed implementation of `RecipeRepository`.
 * @details The referenced `DBManager` is non-owning and must outlive this
 * repository. Callers must serialize access to this object and its
 * `DBManager`, because prepared statements are mutable.
 */
class SqliteRecipeRepository final : public RecipeRepository {
 public:
    /**
     * @brief Creates a repository over an existing database manager.
     * @param db_manager Database manager used for all SQLite operations.
     * @pre `db_manager` outlives this repository.
     */
    explicit SqliteRecipeRepository(db::DBManager& db_manager) noexcept;

    [[nodiscard]] std::expected<types::Product, Error> CreateProduct(
        const types::Product& product) override;
    [[nodiscard]] std::expected<std::optional<types::Product>, Error>
    GetProduct(int64_t product_id) override;
    [[nodiscard]] std::expected<std::vector<types::Product>, Error>
    GetProducts() override;
    [[nodiscard]] std::expected<bool, Error> UpdateProduct(
        int64_t product_id, const types::Product& product) override;
    [[nodiscard]] std::expected<bool, Error> DeleteProduct(
        int64_t product_id) override;

    [[nodiscard]] std::expected<types::Recipe, Error> CreateRecipe(
        const types::Recipe& recipe) override;
    [[nodiscard]] std::expected<std::optional<types::Recipe>, Error> GetRecipe(
        int64_t recipe_id) override;
    [[nodiscard]] std::expected<std::vector<types::Recipe>, Error> GetRecipes()
        override;
    [[nodiscard]] std::expected<std::vector<types::Recipe>, Error>
    GetCookableRecipes() override;
    [[nodiscard]] std::expected<bool, Error> UpdateRecipe(
        int64_t recipe_id, const types::Recipe& recipe) override;
    [[nodiscard]] std::expected<bool, Error> DeleteRecipe(
        int64_t recipe_id) override;

 private:
    db::DBManager* db_manager_;
};

}  // namespace app