#include "DBManager.hpp"

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"

#include "types/kitchen/Types.hpp"
#include "types/SQL/Types.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <optional>
#include <span>
#include <sstream>
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

[[nodiscard]] std::string ToDateString(std::chrono::sys_days date) {
    const std::chrono::year_month_day ymd{date};
    return std::format("{:04d}-{:02d}-{:02d}", static_cast<int>(ymd.year()),
                       static_cast<unsigned>(ymd.month()),
                       static_cast<unsigned>(ymd.day()));
}

[[nodiscard]] std::chrono::sys_days FromDateString(
    const std::string& date_str) {
    std::chrono::sys_days result;
    std::istringstream ss(date_str);
    std::chrono::from_stream(ss, "%Y-%m-%d", result);
    return result;
}

void BindProduct(Statement& statement, const Product& product) {
    const Dates& dates = product.GetDates();
    statement.bind(1, product.GetName());
    statement.bind(2, static_cast<uint8_t>(product.GetDimension()));
    statement.bind(3, product.GetAmount());

    if (dates.manufacture.has_value()) {
        statement.bind(4, ToDateString(dates.manufacture.value()));
    } else {
        statement.bind(4, nullptr);
    }

    if (dates.expiration.has_value()) {
        statement.bind(5, ToDateString(dates.expiration.value()));
    } else {
        statement.bind(5, nullptr);
    }
}

[[nodiscard]] Product ReadProduct(const Statement& statement) {
    const int64_t product_id = statement.getColumn(0).getInt64();
    const std::string name = statement.getColumn(1).getString();
    const auto dimension =
        static_cast<Dimension>(statement.getColumn(2).getInt());
    const int amount = statement.getColumn(3).getInt();

    std::optional<std::chrono::sys_days> manufacture_days = std::nullopt;
    std::optional<std::chrono::sys_days> expiration_days = std::nullopt;
    if (!statement.getColumn(4).isNull()) {
        manufacture_days = FromDateString(statement.getColumn(4).getString());
    }
    if (!statement.getColumn(5).isNull()) {
        expiration_days = FromDateString(statement.getColumn(5).getString());
    }

    return Product{
        name, amount, dimension,
        Dates{.manufacture = manufacture_days, .expiration = expiration_days},
        product_id};
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
      create_product_(db_, std::string{PreparedStatements::kCreateProduct}),
      select_all_products_(db_,
                           std::string{PreparedStatements::kSelectAllProducts}),
      select_product_by_id_(
          db_, std::string{PreparedStatements::kSelectProductById}),
      update_product_by_id_(
          db_, std::string{PreparedStatements::kUpdateProductById}),
      delete_product_by_id_(
          db_, std::string{PreparedStatements::kDeleteProductById}),
      insert_recipe_if_absent_(
          db_, std::string{PreparedStatements::kInsertRecipeIfAbsent}),
      create_recipe_(db_, std::string{PreparedStatements::kCreateRecipe}),
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
      select_recipe_by_id_with_ingredients_(
          db_,
          std::string{PreparedStatements::kSelectRecipeByIdWithIngredients}),
      update_recipe_name_by_id_(
          db_, std::string{PreparedStatements::kUpdateRecipeNameById}),
      delete_recipe_by_id_(db_,
                           std::string{PreparedStatements::kDeleteRecipeById}),
      select_cookable_recipes_with_ingredients_(
          db_,
          std::string{
              PreparedStatements::kSelectCookableRecipesWithIngredients}) {}

//////// Public Methods ////////

void DBManager::InsertProduct(const types::Product& product) {
    ResetStatement(insert_product_);
    BindProduct(insert_product_, product);
    insert_product_.exec();
}

void DBManager::InsertProducts(std::span<const Product> products) {
    Transaction transaction(db_);
    for (const auto& product : products) {
        InsertProduct(product);
    }
    transaction.commit();
}

int64_t DBManager::CreateProduct(const Product& product) {
    ResetStatement(create_product_);
    BindProduct(create_product_, product);
    create_product_.exec();
    return db_.getLastInsertRowid();
}

std::optional<Product> DBManager::GetProduct(int64_t product_id) {
    ResetStatement(select_product_by_id_);
    select_product_by_id_.bind(1, product_id);
    if (!select_product_by_id_.executeStep()) {
        return std::nullopt;
    }
    return ReadProduct(select_product_by_id_);
}

bool DBManager::UpdateProduct(int64_t product_id, const Product& product) {
    ResetStatement(update_product_by_id_);
    BindProduct(update_product_by_id_, product);
    update_product_by_id_.bind(6, product_id);
    update_product_by_id_.exec();
    return db_.getChanges() == 1;
}

bool DBManager::DeleteProduct(int64_t product_id) {
    ResetStatement(delete_product_by_id_);
    delete_product_by_id_.bind(1, product_id);
    delete_product_by_id_.exec();
    return db_.getChanges() == 1;
}

std::vector<types::Product> DBManager::GetAllProducts() {
    ResetStatement(select_all_products_);

    std::vector<Product> products;
    while (select_all_products_.executeStep()) {
        products.push_back(ReadProduct(select_all_products_));
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
    Transaction transaction(db_);
    ResetStatement(insert_recipe_if_absent_);
    insert_recipe_if_absent_.bind(1, std::string{recipe.GetName()});
    insert_recipe_if_absent_.exec();

    ResetStatement(select_recipe_id_by_name_);
    select_recipe_id_by_name_.bind(1, std::string{recipe.GetName()});
    if (!select_recipe_id_by_name_.executeStep()) {
        throw std::runtime_error("Failed to retrieve recipe id after insert");
    }
    const int64_t recipe_id = select_recipe_id_by_name_.getColumn(0).getInt64();

    ReplaceRecipeIngredients(recipe_id, recipe);
    transaction.commit();
}

void DBManager::InsertRecipes(std::span<const types::Recipe> recipes) {
    Transaction transaction(db_);
    for (const auto& recipe : recipes) {
        ResetStatement(insert_recipe_if_absent_);
        insert_recipe_if_absent_.bind(1, std::string{recipe.GetName()});
        insert_recipe_if_absent_.exec();

        ResetStatement(select_recipe_id_by_name_);
        select_recipe_id_by_name_.bind(1, std::string{recipe.GetName()});
        if (!select_recipe_id_by_name_.executeStep()) {
            throw std::runtime_error(
                "Failed to retrieve recipe id after insert");
        }
        ReplaceRecipeIngredients(
            select_recipe_id_by_name_.getColumn(0).getInt64(), recipe);
    }
    transaction.commit();
}

int64_t DBManager::CreateRecipe(const Recipe& recipe) {
    Transaction transaction(db_);
    ResetStatement(create_recipe_);
    create_recipe_.bind(1, std::string{recipe.GetName()});
    create_recipe_.exec();
    const int64_t recipe_id = db_.getLastInsertRowid();
    ReplaceRecipeIngredients(recipe_id, recipe);
    transaction.commit();
    return recipe_id;
}

std::optional<Recipe> DBManager::GetRecipe(int64_t recipe_id) {
    const std::vector<Recipe> recipes =
        FetchRecipes(select_recipe_by_id_with_ingredients_, recipe_id);
    if (recipes.empty()) {
        return std::nullopt;
    }
    return recipes.front();
}

bool DBManager::UpdateRecipe(int64_t recipe_id, const Recipe& recipe) {
    Transaction transaction(db_);
    ResetStatement(update_recipe_name_by_id_);
    update_recipe_name_by_id_.bind(1, std::string{recipe.GetName()});
    update_recipe_name_by_id_.bind(2, recipe_id);
    update_recipe_name_by_id_.exec();
    if (db_.getChanges() != 1) {
        return false;
    }

    ReplaceRecipeIngredients(recipe_id, recipe);
    transaction.commit();
    return true;
}

bool DBManager::DeleteRecipe(int64_t recipe_id) {
    ResetStatement(delete_recipe_by_id_);
    delete_recipe_by_id_.bind(1, recipe_id);
    delete_recipe_by_id_.exec();
    return db_.getChanges() == 1;
}

void DBManager::ReplaceRecipeIngredients(int64_t recipe_id,
                                         const Recipe& recipe) {
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

std::vector<types::Recipe> DBManager::GetAllRecipes() {
    return FetchRecipes(select_all_recipes_with_ingredients_);
}

std::vector<types::Recipe> DBManager::GetCookableRecipes() {
    return FetchRecipes(select_cookable_recipes_with_ingredients_);
}

//////// Private Methods ////////

std::vector<types::Recipe> DBManager::FetchRecipes(
    Statement& stmt, std::optional<int64_t> recipe_id) {
    ResetStatement(stmt);
    if (recipe_id.has_value()) {
        stmt.bind(1, recipe_id.value());
    }
    std::vector<Recipe> recipes;
    int64_t current_id = -1;

    while (stmt.executeStep()) {
        const int64_t recipe_id = stmt.getColumn(0).getInt64();
        const std::string recipe_name = stmt.getColumn(1).getString();

        if (recipe_id != current_id) {
            recipes.emplace_back(recipe_name, std::initializer_list<Product>{},
                                 recipe_id);
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
