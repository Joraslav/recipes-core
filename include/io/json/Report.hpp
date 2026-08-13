#pragma once

#include "concepts/Concepts.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <filesystem>
#include <span>
#include <system_error>

namespace io::json {

/**
 * @brief Writes products as regular JSON to a file
 * @param products Span of products to serialize
 * @param out_path Output file path. If empty, an exception is thrown.
 */
[[nodiscard]] std::expected<void, std::error_code> WriteProductsJson(
    std::span<const types::Product> products,
    const std::filesystem::path& out_path = {});

/**
 * @brief Writes recipes as regular JSON to a file
 * @param recipes Span of recipes to serialize
 * @param out_path Output file path. If empty, an exception is thrown.
 */
[[nodiscard]] std::expected<void, std::error_code> WriteRecipesJson(
    std::span<const types::Recipe> recipes,
    const std::filesystem::path& out_path = {});

/**
 * @brief Writes items (products or recipes) as JSON to a file
 *
 * Dispatches to either `WriteProductsJson` or `WriteRecipesJson` based on
 * the template parameter type using compile-time polymorphism.
 *
 * @tparam Tv Type of items: `types::Product` or `types::Recipe`
 * @param items Span of items to serialize
 * @param out_path Output file path. If empty, returns error with
 *                 `std::errc::invalid_argument`
 * @return `std::expected<void, std::error_code>` — success or error code
 *         from serialization or file I/O operations
 *
 * @pre `Tv` must satisfy `concepts::ProductOrRecipe`
 * @exception_safety Basic guarantee. File is created/truncated only on
 *                    successful serialization
 *
 * @note This is a template-only function compiled in the header
 */
template <concepts::ProductOrRecipe Tv>
[[nodiscard]] std::expected<void, std::error_code> WriteItemsJson(
    std::span<const Tv> items, const std::filesystem::path& out_path = {}) {
    if constexpr (std::is_same_v<Tv, types::Product>) {
        return WriteProductsJson(items, out_path);
    } else if constexpr (std::is_same_v<Tv, types::Recipe>) {
        return WriteRecipesJson(items, out_path);
    } else {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }
}

}  // namespace io::json
