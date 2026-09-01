#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace api {

/**
 * @brief JSON payload for creating or replacing a product.
 * @details Dates use the ISO 8601 calendar-date format `YYYY-MM-DD` when set.
 */
struct ProductRequest final {
    std::string name;
    int amount{};
    std::string dimension;
    std::optional<std::string> manufacture;
    std::optional<std::string> expiration;
};

/**
 * @brief JSON representation of a persisted product.
 * @details `id` is assigned by the storage layer and is stable for the
 * lifetime of the product.
 */
struct ProductResponse final {
    int64_t id{};
    std::string name;
    int amount{};
    std::string dimension;
    std::optional<std::string> manufacture;
    std::optional<std::string> expiration;
};

/**
 * @brief JSON payload for creating or replacing a recipe.
 */
struct RecipeRequest final {
    std::string name;
    std::vector<ProductRequest> ingredients;
};

/**
 * @brief JSON representation of a persisted recipe and its ingredients.
 */
struct RecipeResponse final {
    int64_t id{};
    std::string name;
    std::vector<ProductResponse> ingredients;
};

/**
 * @brief Stable JSON error payload returned by the API.
 * @details `code` is intended for programmatic handling, while `message` is
 * safe for display to an API consumer.
 */
struct ErrorResponse final {
    std::string code;
    std::string message;
};

}  // namespace api