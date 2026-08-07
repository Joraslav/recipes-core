#pragma once

#include "types/kitchen/Types.hpp"

#include <iostream>
#include <ostream>
#include <span>

namespace io::cli {

/**
 * @brief Prints detailed product information to the output stream
 * @param products Span of products to display
 * @param out Output stream
 * @note If expiration date is set, freshness status is shown
 * @see PrintRecipes
 */
void PrintProducts(std::span<const types::Product> products,
                   std::ostream& out = std::cout);

/**
 * @brief Prints recipe information with optional ingredient details
 * @param recipes Span of recipes to display
 * @param is_full_info If true, prints full ingredient details; if false, prints
 * only count
 * @param out Output stream
 * @see PrintProducts
 */
void PrintRecipes(std::span<const types::Recipe> recipes,
                  bool is_full_info = false, std::ostream& out = std::cout);

}  // namespace io::cli
