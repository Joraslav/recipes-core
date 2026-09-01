#pragma once

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"

#include "types/kitchen/Types.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace db {

/**
 * @brief Manages SQLite database operations for products and recipes.
 *
 * Provides CRUD operations for products and recipes, with support for
 * batch inserts and queries. Implements RAII and is non-copyable.
 *
 * Key features:
 * - Insert products and recipes (single or batch)
 * - Retrieve all products, all recipes, or cookable recipes
 * - Query ingredients for a specific recipe
 *
 * All queries use prepared statements for performance and security.
 */
class DBManager final {
 public:
    using Statement = SQLite::Statement;
    using Database = SQLite::Database;

    explicit DBManager(std::string_view db_path);
    DBManager();
    ~DBManager() = default;

    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    DBManager(DBManager&&) = default;
    DBManager& operator=(DBManager&&) = default;

    void InsertProduct(const types::Product& product);
    void InsertProducts(std::span<const types::Product> products);
    [[nodiscard]] int64_t CreateProduct(const types::Product& product);
    [[nodiscard]] std::optional<types::Product> GetProduct(int64_t product_id);
    [[nodiscard]] bool UpdateProduct(int64_t product_id,
                                     const types::Product& product);
    [[nodiscard]] bool DeleteProduct(int64_t product_id);

    void InsertRecipe(const types::Recipe& recipe);
    void InsertRecipes(std::span<const types::Recipe> recipes);
    [[nodiscard]] int64_t CreateRecipe(const types::Recipe& recipe);
    [[nodiscard]] std::optional<types::Recipe> GetRecipe(int64_t recipe_id);
    [[nodiscard]] bool UpdateRecipe(int64_t recipe_id,
                                    const types::Recipe& recipe);
    [[nodiscard]] bool DeleteRecipe(int64_t recipe_id);

    [[nodiscard]] std::vector<types::Product> GetAllProducts();

    [[nodiscard]] std::vector<types::Recipe> GetAllRecipes();

    [[nodiscard]] std::vector<types::Recipe> GetCookableRecipes();

    [[nodiscard]] std::vector<types::Product> GetRecipeIngredients(
        int64_t recipe_id);

 private:
    // db_ must be declared first: all Statement members reference it.
    Database db_;

    Statement insert_product_;
    Statement create_product_;
    Statement select_all_products_;
    Statement select_product_by_id_;
    Statement update_product_by_id_;
    Statement delete_product_by_id_;
    Statement insert_recipe_if_absent_;
    Statement create_recipe_;
    Statement select_recipe_id_by_name_;
    Statement delete_recipe_ingredients_by_recipe_id_;
    Statement insert_recipe_ingredient_;
    Statement select_recipe_ingredients_by_recipe_id_;
    Statement select_all_recipes_with_ingredients_;
    Statement select_recipe_by_id_with_ingredients_;
    Statement update_recipe_name_by_id_;
    Statement delete_recipe_by_id_;
    Statement select_cookable_recipes_with_ingredients_;

    /**
     * @brief Delegating constructor: accepts an already-initialized Database
     *        (schema created) and initialises all prepared statements.
     *
     * Two-phase init pattern: the public constructors create the schema via
     * a static helper, then delegate here so that the member initializer list
     * can construct every Statement directly — no std::optional needed.
     */
    explicit DBManager(Database db);

    [[nodiscard]] static Database InitDb(std::string_view db_path);

    void ReplaceRecipeIngredients(int64_t recipe_id,
                                  const types::Recipe& recipe);
    [[nodiscard]] static std::vector<types::Recipe> FetchRecipes(
        Statement& stmt, std::optional<int64_t> recipe_id = std::nullopt);
};

}  // namespace db
