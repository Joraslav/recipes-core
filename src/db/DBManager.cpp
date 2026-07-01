#include "DBManager.hpp"

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"

#include "types/kitchen/Types.hpp"
#include "types/SQL/Types.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using types::Dates;
using types::Dimension;
using types::PreparedStatements;
using types::Product;
using types::Recipe;
using Statement = db::DBManager::Statement;
using Database = db::DBManager::Database;
using Transaction = SQLite::Transaction;

namespace {

void ResetStatement(Statement& statement) {
    statement.reset();
    statement.clearBindings();
}

[[nodiscard]] int64_t ToDaysSinceEpoch(std::chrono::sys_days date) noexcept {
    return date.time_since_epoch().count();
}

[[nodiscard]] std::chrono::sys_days FromStoredDays(
    int64_t stored_days) noexcept {
    return std::chrono::sys_days{
        std::chrono::days{stored_days}};  // NOLINT (missing-includes)
}

}  // namespace

namespace db {

//////// Private static helpers ////////

Database DBManager::InitDb(std::string_view db_path) {
    Database db(db_path,
                SQLite::OPEN_READWRITE |  // NOLINT(hicpp-signed-bitwise)
                    SQLite::OPEN_CREATE | SQLite::OPEN_FULLMUTEX);
    db.exec("PRAGMA foreign_keys = ON;");
    db.exec(std::string{PreparedStatements::kCreateProductsTable});
    db.exec(std::string{PreparedStatements::kCreateRecipesTable});
    db.exec(std::string{PreparedStatements::kCreateRecipeIngredientsTable});
    return db;
}

//////// Constructors ////////

DBManager::DBManager() : DBManager(InitDb(":memory:")) {}

DBManager::DBManager(std::string_view db_path) : DBManager(InitDb(db_path)) {}

// Delegating constructor: schema already applied, initialise all statements.
DBManager::DBManager(Database db)
    : db_(std::move(db)),
      insert_product_(db_, std::string{PreparedStatements::kInsertProduct}),
      select_all_products_(db_,
                           std::string{PreparedStatements::kSelectAllProducts}),
      insert_recipe_if_absent_(
          db_, std::string{PreparedStatements::kInsertRecipeIfAbsent}),
      select_recipe_id_by_name_(
          db_, std::string{PreparedStatements::kSelectRecipeIdByName}),
      delete_recipe_ingredients_by_recipe_id_(
          db_,
          std::string{PreparedStatements::kDeleteRecipeIngredientsByRecipeId}),
      insert_recipe_ingredient_(
          db_, std::string{PreparedStatements::kInsertRecipeIngredient}),
      select_recipe_ingredients_by_recipe_id_(
          db_,
          std::string{PreparedStatements::kSelectRecipeIngredientsByRecipeId}),
      select_all_recipes_with_ingredients_(
          db_,
          std::string{PreparedStatements::kSelectAllRecipesWithIngredients}),
      select_cookable_recipes_with_ingredients_(
          db_,
          std::string{
              PreparedStatements::kSelectCookableRecipesWithIngredients}) {}

//////// Public Methods ////////

void DBManager::InsertProduct(const types::Product& product) {
    ResetStatement(insert_product_);

    const Dates& dates = product.GetDates();
    insert_product_.bind(1, product.GetName());
    insert_product_.bind(2, static_cast<uint8_t>(product.GetDimension()));
    insert_product_.bind(3, product.GetAmount());

    if (dates.manufacture.has_value()) {
        insert_product_.bind(4, ToDaysSinceEpoch(dates.manufacture.value()));
    } else {
        insert_product_.bind(4, nullptr);
    }

    if (dates.expiration.has_value()) {
        insert_product_.bind(5, ToDaysSinceEpoch(dates.expiration.value()));
    } else {
        insert_product_.bind(5, nullptr);
    }

    insert_product_.exec();
}

void DBManager::InsertProducts(std::span<const Product> products) {
    Transaction transaction(db_);
    for (const auto& product : products) {
        InsertProduct(product);
    }
    transaction.commit();
}

std::vector<types::Product> DBManager::GetAllProducts() {
    ResetStatement(select_all_products_);

    std::vector<Product> products;
    while (select_all_products_.executeStep()) {
        const std::string name = select_all_products_.getColumn(0).getString();
        const auto dimension =
            static_cast<Dimension>(select_all_products_.getColumn(1).getInt());
        const int amount = select_all_products_.getColumn(2).getInt();

        std::optional<std::chrono::sys_days> manufacture_days = std::nullopt;
        std::optional<std::chrono::sys_days> expiration_days = std::nullopt;
        if (!select_all_products_.getColumn(3).isNull()) {
            manufacture_days =
                FromStoredDays(select_all_products_.getColumn(3).getInt64());
        }
        if (!select_all_products_.getColumn(4).isNull()) {
            expiration_days =
                FromStoredDays(select_all_products_.getColumn(4).getInt64());
        }

        products.emplace_back(name, amount, dimension,
                              Dates{.manufacture = manufacture_days,
                                    .expiration = expiration_days});
    }
    return products;
}

std::vector<types::Product> DBManager::GetRecipeIngredients(int64_t recipe_id) {
    ResetStatement(select_recipe_ingredients_by_recipe_id_);
    select_recipe_ingredients_by_recipe_id_.bind(1, recipe_id);

    std::vector<Product> ingredients;
    while (select_recipe_ingredients_by_recipe_id_.executeStep()) {
        const std::string name =
            select_recipe_ingredients_by_recipe_id_.getColumn(0).getString();
        const Dimension dimension = static_cast<Dimension>(
            select_recipe_ingredients_by_recipe_id_.getColumn(1).getInt());
        const int amount =
            select_recipe_ingredients_by_recipe_id_.getColumn(2).getInt();
        ingredients.emplace_back(name, amount, dimension);
    }
    return ingredients;
}

void DBManager::InsertRecipe(const types::Recipe& recipe) {
    ResetStatement(insert_recipe_if_absent_);
    insert_recipe_if_absent_.bind(1, std::string{recipe.GetName()});
    insert_recipe_if_absent_.exec();

    ResetStatement(select_recipe_id_by_name_);
    select_recipe_id_by_name_.bind(1, std::string{recipe.GetName()});
    if (!select_recipe_id_by_name_.executeStep()) {
        throw std::runtime_error("Failed to retrieve recipe id after insert");
    }
    const int64_t recipe_id = select_recipe_id_by_name_.getColumn(0).getInt64();

    ResetStatement(delete_recipe_ingredients_by_recipe_id_);
    delete_recipe_ingredients_by_recipe_id_.bind(1, recipe_id);
    delete_recipe_ingredients_by_recipe_id_.exec();

    for (const auto& ingredient : recipe.GetIngredients()) {
        ResetStatement(insert_recipe_ingredient_);
        insert_recipe_ingredient_.bind(1, recipe_id);
        insert_recipe_ingredient_.bind(2, ingredient.GetName());
        insert_recipe_ingredient_.bind(
            3, static_cast<uint8_t>(ingredient.GetDimension()));
        insert_recipe_ingredient_.bind(4, ingredient.GetAmount());
        insert_recipe_ingredient_.exec();
    }
}

void DBManager::InsertRecipes(std::span<const types::Recipe> recipes) {
    Transaction transaction(db_);
    for (const auto& recipe : recipes) {
        InsertRecipe(recipe);
    }
    transaction.commit();
}

std::vector<types::Recipe> DBManager::GetAllRecipes() {
    return FetchRecipes(select_all_recipes_with_ingredients_);
}

std::vector<types::Recipe> DBManager::GetCookableRecipes() {
    return FetchRecipes(select_cookable_recipes_with_ingredients_);
}

//////// Private Methods ////////

std::vector<types::Recipe> DBManager::FetchRecipes(Statement& stmt) {
    ResetStatement(stmt);
    std::vector<Recipe> recipes;
    int64_t current_id = -1;

    while (stmt.executeStep()) {
        const int64_t recipe_id = stmt.getColumn(0).getInt64();
        const std::string recipe_name = stmt.getColumn(1).getString();

        if (recipe_id != current_id) {
            recipes.emplace_back(recipe_name);
            current_id = recipe_id;
        }

        if (!stmt.getColumn(2).isNull()) {
            const std::string product_name = stmt.getColumn(2).getString();
            const auto dimension =
                static_cast<Dimension>(stmt.getColumn(3).getInt());
            const int amount = stmt.getColumn(4).getInt();
            recipes.back().AddIngredient(
                Product{product_name, amount, dimension});
        }
    }
    return recipes;
}

}  // namespace db
