#pragma once

#include "concepts/Concepts.hpp"
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
 * @see PrintRecipes, PrintItems
 */
void PrintProducts(std::span<const types::Product> products,
                   std::ostream& out = std::cout);

/**
 * @brief Prints recipe information with optional ingredient details
 * @param recipes Span of recipes to display
 * @param is_full_info If true, prints full ingredient details; if false, prints
 * only count
 * @param out Output stream
 * @see PrintProducts, PrintItems
 */
void PrintRecipes(std::span<const types::Recipe> recipes,
                  bool is_full_info = false, std::ostream& out = std::cout);

/**
 * @brief Prints items (products or recipes) to output stream
 *
 * Dispatches to either `PrintProducts` or `PrintRecipes` based on the
 * template parameter type using compile-time polymorphism.
 *
 * @tparam Tv Type of items: `types::Product` or `types::Recipe`
 * @param items Span of items to display
 * @param is_full_info Only used for recipes: if true, prints full ingredient
 *                     details; if false, prints only count (default: false)
 * @param out Output stream (default: `std::cout`)
 *
 * @pre `Tv` must satisfy `concepts::ProductOrRecipe`
 * @post Output is written to `out` stream
 *
 * @note This is a template-only function compiled in the header
 * @see PrintProducts, PrintRecipes
 */
template <concepts::ProductOrRecipe Tv>
void PrintItems(std::span<const Tv> items, bool is_full_info = false,
                std::ostream& out = std::cout) {
    if constexpr (std::is_same_v<Tv, types::Product>) {
        PrintProducts(items, out);
    } else if constexpr (std::is_same_v<Tv, types::Recipe>) {
        PrintRecipes(items, is_full_info, out);
    }
}

}  // namespace io::cli
