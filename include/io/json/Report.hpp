#pragma once

#include "types/kitchen/Types.hpp"

#include <filesystem>
#include <span>

namespace io::json {

/**
 * @brief Writes products as regular JSON to a file
 * @param products Span of products to serialize
 * @param out_path Output file path. If empty, an exception is thrown.
 */
void WriteProductsJson(std::span<const types::Product> products,
                       const std::filesystem::path& out_path = {});

/**
 * @brief Writes recipes as regular JSON to a file
 * @param recipes Span of recipes to serialize
 * @param out_path Output file path. If empty, an exception is thrown.
 */
void WriteRecipesJson(std::span<const types::Recipe> recipes,
                      const std::filesystem::path& out_path = {});

}  // namespace io::json
