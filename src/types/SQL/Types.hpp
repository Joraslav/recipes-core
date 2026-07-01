#pragma once

#include <string_view>

namespace types {

/*
 * @brief Prepared statements for SQL
 */
struct PreparedStatements final {
    static constexpr std::string_view kCreateProductsTable = R"sql(
CREATE TABLE IF NOT EXISTS products (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    dimension INTEGER NOT NULL,
    amount_base INTEGER NOT NULL CHECK (amount_base >= 0),
    manufacture_date DATETIME,
    expiration_date DATETIME,
UNIQUE(name, dimension)
);
)sql";

    static constexpr std::string_view kCreateRecipesTable = R"sql(
CREATE TABLE IF NOT EXISTS recipes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);
)sql";

    static constexpr std::string_view kCreateRecipeIngredientsTable = R"sql(
CREATE TABLE IF NOT EXISTS recipe_ingredients (
    recipe_id INTEGER NOT NULL,
    product_name TEXT NOT NULL,
    dimension INTEGER NOT NULL,
    required_amount_base INTEGER NOT NULL CHECK (required_amount_base >= 0),
PRIMARY KEY(recipe_id, product_name, dimension),
FOREIGN KEY(recipe_id) REFERENCES recipes(id) ON DELETE CASCADE
);
)sql";

    static constexpr std::string_view kInsertProduct = R"sql(
INSERT INTO products(name, dimension, amount_base, manufacture_date, expiration_date)
VALUES (?, ?, ?, ?, ?)
ON CONFLICT(name, dimension) DO UPDATE SET
    amount_base = excluded.amount_base,
    manufacture_date = excluded.manufacture_date,
    expiration_date = excluded.expiration_date;
)sql";

    static constexpr std::string_view kSelectAllProducts = R"sql(
SELECT name, dimension, amount_base, manufacture_date, expiration_date
FROM products
ORDER BY amount_base;
)sql";

    static constexpr std::string_view kInsertRecipeIfAbsent = R"sql(
INSERT OR IGNORE INTO recipes(name) VALUES (?);
)sql";

    static constexpr std::string_view kSelectRecipeIdByName = R"sql(
SELECT id FROM recipes WHERE name = ?;
)sql";

    static constexpr std::string_view kDeleteRecipeIngredientsByRecipeId =
        R"sql(
DELETE FROM recipe_ingredients WHERE recipe_id = ?;
)sql";

    static constexpr std::string_view kInsertRecipeIngredient =
        R"sql(
INSERT INTO recipe_ingredients(recipe_id, product_name, dimension, required_amount_base)
VALUES (?, ?, ?, ?)
ON CONFLICT(recipe_id, product_name, dimension) DO UPDATE SET
        required_amount_base = excluded.required_amount_base;
)sql";

    static constexpr std::string_view kSelectRecipeIngredientsByRecipeId =
        R"sql(
SELECT product_name, dimension, required_amount_base
FROM recipe_ingredients
WHERE recipe_id = ?
ORDER BY product_name, dimension;
)sql";

    static constexpr std::string_view kSelectAllRecipesWithIngredients = R"sql(
SELECT r.id, r.name, ri.product_name, ri.dimension, ri.required_amount_base
FROM recipes r
LEFT JOIN recipe_ingredients ri ON ri.recipe_id = r.id
ORDER BY r.name, ri.product_name, ri.dimension;
)sql";

    static constexpr std::string_view kSelectCookableRecipesWithIngredients =
        R"sql(
SELECT r.id, r.name, ri.product_name, ri.dimension, ri.required_amount_base
FROM recipes r
LEFT JOIN recipe_ingredients ri ON ri.recipe_id = r.id
WHERE NOT EXISTS (
    SELECT 1
    FROM recipe_ingredients ri2
    LEFT JOIN products p
      ON p.name = ri2.product_name
     AND p.dimension = ri2.dimension
    WHERE ri2.recipe_id = r.id
      AND (p.id IS NULL OR p.amount_base < ri2.required_amount_base)
)
ORDER BY r.name, ri.product_name, ri.dimension;
)sql";
};

}  // namespace types
