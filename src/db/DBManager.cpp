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
#include <vector>

using namespace types;
using Statement = db::DBManager::Statement;
using Transaction = SQLite::Transaction;

namespace {

void ResetStatement(Statement& statement) {
    statement.reset();
    statement.clearBindings();
}

[[nodiscard]] int64_t ToDaysSinceEpoch(std::chrono::sys_days date) noexcept {
    return date.time_since_epoch().count();
}

[[nodiscard]] std::chrono::sys_days FromStoredDays(int64_t stored_days) noexcept {
    return std::chrono::sys_days{
        std::chrono::days{stored_days}};  // NOLINT (missing-includes)
}

}  // namespace

namespace db {

//////// Public Methods ////////

DBManager::DBManager() : DBManager(":memory:") {}

DBManager::DBManager(std::string_view db_path)
    : db_(db_path, SQLite::OPEN_READWRITE |  // NOLINT(hicpp-signed-bitwise)
                       SQLite::OPEN_CREATE | SQLite::OPEN_FULLMUTEX) {

    CreateSchema();
    PrepareStatements();
}

void DBManager::InsertProduct(const types::Product& product) {
    Statement& insert_statement = insert_product_.value();
    ResetStatement(insert_statement);

    const Dates& dates = product.GetDates();
    insert_statement.bind(1, product.GetName());
    insert_statement.bind(2, static_cast<uint8_t>(product.GetDimension()));
    insert_statement.bind(3, product.GetAmount());

    if (dates.manufacture.has_value()) {
        insert_statement.bind(4, ToDaysSinceEpoch(dates.manufacture.value()));
    } else {
        insert_statement.bind(4, nullptr);
    }

    if (dates.expiration.has_value()) {
        insert_statement.bind(5, ToDaysSinceEpoch(dates.expiration.value()));
    } else {
        insert_statement.bind(5, nullptr);
    }

    insert_statement.exec();
}

void DBManager::InsertProducts(std::span<const Product> products) {
    Transaction transaction(db_);
    for (const auto& product : products) {
        InsertProduct(product);
    }
    transaction.commit();
}

std::vector<types::Product> DBManager::GetAllProducts() {
    Statement& get_statement = select_all_products_.value();
    ResetStatement(get_statement);

    std::vector<Product> products;
    while (get_statement.executeStep()) {
        const std::string name = get_statement.getColumn(0).getString();
        const auto dimension =
            static_cast<Dimension>(get_statement.getColumn(1).getInt());
        const int amount = get_statement.getColumn(2).getInt();

        std::optional<std::chrono::sys_days> manufacture_days = std::nullopt;
        std::optional<std::chrono::sys_days> expiration_days = std::nullopt;
        if (!get_statement.getColumn(3).isNull()) {
            manufacture_days =
                FromStoredDays(get_statement.getColumn(3).getInt64());
        }
        if (!get_statement.getColumn(4).isNull()) {
            expiration_days =
                FromStoredDays(get_statement.getColumn(4).getInt64());
        }

        products.emplace_back(name, amount, dimension,
                              Dates{.manufacture = manufacture_days,
                                    .expiration = expiration_days});
    }
    return products;
}

std::vector<types::Product> DBManager::GetRecipeIngredients(int64_t recipe_id) {
    Statement& get_statement = select_recipe_ingredients_by_recipe_id_.value();
    ResetStatement(get_statement);
    get_statement.bind(1, recipe_id);

    std::vector<Product> ingredients;
    while (get_statement.executeStep()) {
        const std::string name = get_statement.getColumn(0).getString();
        const Dimension dimension =
            static_cast<Dimension>(get_statement.getColumn(1).getInt());
        const int amount = get_statement.getColumn(2).getInt();
        ingredients.emplace_back(name, amount, dimension);
    }
    return ingredients;
}

void DBManager::InsertRecipe(const types::Recipe& recipe) {
    Statement& insert_recipe_stmt = insert_recipe_if_absent_.value();
    ResetStatement(insert_recipe_stmt);
    insert_recipe_stmt.bind(1, std::string{recipe.GetName()});
    insert_recipe_stmt.exec();

    Statement& select_id_stmt = select_recipe_id_by_name_.value();
    ResetStatement(select_id_stmt);
    select_id_stmt.bind(1, std::string{recipe.GetName()});
    if (!select_id_stmt.executeStep()) {
        throw std::runtime_error("Failed to retrieve recipe id after insert");
    }
    const int64_t recipe_id = select_id_stmt.getColumn(0).getInt64();

    Statement& delete_ingredients_stmt =
        delete_recipe_ingredients_by_recipe_id_.value();
    ResetStatement(delete_ingredients_stmt);
    delete_ingredients_stmt.bind(1, recipe_id);
    delete_ingredients_stmt.exec();

    Statement& insert_ingredient_stmt = insert_recipe_ingredient_.value();
    for (const auto& ingredient : recipe.GetIngredients()) {
        ResetStatement(insert_ingredient_stmt);
        insert_ingredient_stmt.bind(1, recipe_id);
        insert_ingredient_stmt.bind(2, ingredient.GetName());
        insert_ingredient_stmt.bind(
            3, static_cast<uint8_t>(ingredient.GetDimension()));
        insert_ingredient_stmt.bind(4, ingredient.GetAmount());
        insert_ingredient_stmt.exec();
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
    return FetchRecipes(select_all_recipes_with_ingredients_.value());
}

std::vector<types::Recipe> DBManager::GetCookableRecipes() {
    return FetchRecipes(select_cookable_recipes_with_ingredients_.value());
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
            recipes.back().AddIngredient(Product{product_name, amount, dimension});
        }
    }
    return recipes;
}

void DBManager::CreateSchema() {
    db_.exec("PRAGMA foreign_keys = ON;");
    db_.exec(std::string{PreparedStatements::kCreateProductsTable});
    db_.exec(std::string{PreparedStatements::kCreateRecipesTable});
    db_.exec(std::string{PreparedStatements::kCreateRecipeIngredientsTable});
}

void DBManager::PrepareStatements() {
    insert_product_.emplace(db_,
                            std::string{PreparedStatements::kInsertProduct});
    select_all_products_.emplace(
        db_, std::string{PreparedStatements::kSelectAllProducts});
    insert_recipe_if_absent_.emplace(
        db_, std::string{PreparedStatements::kInsertRecipeIfAbsent});
    select_recipe_id_by_name_.emplace(
        db_, std::string{PreparedStatements::kSelectRecipeIdByName});
    delete_recipe_ingredients_by_recipe_id_.emplace(
        db_,
        std::string{PreparedStatements::kDeleteRecipeIngredientsByRecipeId});
    insert_recipe_ingredient_.emplace(
        db_, std::string{PreparedStatements::kInsertRecipeIngredient});
    select_recipe_ingredients_by_recipe_id_.emplace(
        db_,
        std::string{PreparedStatements::kSelectRecipeIngredientsByRecipeId});
    select_all_recipes_with_ingredients_.emplace(
        db_,
        std::string{PreparedStatements::kSelectAllRecipesWithIngredients});
    select_cookable_recipes_with_ingredients_.emplace(
        db_,
        std::string{
            PreparedStatements::kSelectCookableRecipesWithIngredients});
}

}  // namespace db
