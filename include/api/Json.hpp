#pragma once

#include "api/Types.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace api {

/**
 * @brief Parses a product creation or update payload from JSON.
 * @param json JSON document to parse.
 * @return Parsed request or a human-readable JSON syntax/type error.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<ProductRequest, std::string> ParseProductRequest(
    std::string_view json);
/**
 * @brief Parses a recipe creation or update payload from JSON.
 * @param json JSON document to parse.
 * @return Parsed request or a human-readable JSON syntax/type error.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<RecipeRequest, std::string> ParseRecipeRequest(
    std::string_view json);

/**
 * @brief Serializes a persisted product to the API JSON representation.
 * @param response Product response to serialize.
 * @return JSON document or serialization error description.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<std::string, std::string> SerializeJson(
    const ProductResponse& response);
/**
 * @brief Serializes persisted products to the API JSON representation.
 * @param responses Product responses to serialize.
 * @return JSON document or serialization error description.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<std::string, std::string> SerializeJson(
    const std::vector<ProductResponse>& responses);
/**
 * @brief Serializes a persisted recipe to the API JSON representation.
 * @param response Recipe response to serialize.
 * @return JSON document or serialization error description.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<std::string, std::string> SerializeJson(
    const RecipeResponse& response);
/**
 * @brief Serializes persisted recipes to the API JSON representation.
 * @param responses Recipe responses to serialize.
 * @return JSON document or serialization error description.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<std::string, std::string> SerializeJson(
    const std::vector<RecipeResponse>& responses);
/**
 * @brief Serializes a public API error to JSON.
 * @param response Error response to serialize.
 * @return JSON document or serialization error description.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<std::string, std::string> SerializeJson(
    const ErrorResponse& response);

}  // namespace api