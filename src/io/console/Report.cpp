#include "Report.hpp"

#include "types/kitchen/Types.hpp"

// #include <chrono>
#include <format>
#include <ostream>
#include <print>
#include <span>
#include <string_view>

using types::Product;
using types::Recipe;
using namespace std::string_view_literals;

namespace io::cli {

void PrintProducts(std::span<const Product> products, std::ostream& out) {
    if (products.empty()) {
        std::println(out, "No products to display");
        return;
    }

    for (const auto& product : products) {
        std::println(out, "Name: {}"sv, product.GetNameView());
        std::println(out, "Amount: {}{}", product.GetAmount(),
                     product.GetDimensionInString());

        const auto& dates = product.GetDates();
        if (dates.manufacture.has_value()) {
            std::println(out, "Manufacture: {:%d-%m-%Y}",
                         dates.manufacture.value());
        } else {
            std::println(out, "Manufacture: not set");
        }

        if (dates.expiration.has_value()) {
            std::println(out, "Expiration: {:%d-%m-%Y}",
                         dates.expiration.value());
            std::println(out, "Fresh: {}", product.IsFresh() ? "Yes" : "No");
        } else {
            std::println(out, "Expiration: not set");
        }
        std::println(out, "------------------------");
    }
}

void PrintRecipes(std::span<const Recipe> recipes, bool is_full_info,
                  std::ostream& out) {
    if (recipes.empty()) {
        std::println(out, "No recipes to display");
        return;
    }

    for (const auto& recipe : recipes) {
        std::println(out, "Name: {}", recipe.GetNameView());
        if (is_full_info) {
            std::println(out, "Ingredients:");
            PrintProducts(recipe.GetIngredients(), out);
        } else {
            std::println(out, "Ingredients count: {}",
                         recipe.GetIngredients().size());
        }
        std::println(out, "------------------------");
    }
}

}  // namespace io::cli
