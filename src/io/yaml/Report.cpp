#include "Report.hpp"

#include "types/kitchen/Types.hpp"

#include <glaze/core/common.hpp>
// #include <glaze/core/reflect.hpp>
#include <glaze/forward.hpp>
#include <glaze/yaml/write.hpp>

#include <chrono>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using types::Product;
using types::Recipe;
namespace fs = std::filesystem;

namespace {

struct ProductView final {
    std::string name;
    int amount{};
    std::string dimension;
    std::optional<std::string> manufacture;
    std::optional<std::string> expiration;
};

struct RecipeView final {
    std::string name;
    std::vector<ProductView> ingredients;
};

}  // namespace

namespace glz {

template <>
struct meta<ProductView> {
    using T = ProductView;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value = object(
        "name", &T::name, "amount", &T::amount, "dimension", &T::dimension,
        "manufacture", &T::manufacture, "expiration", &T::expiration);
};

template <>
struct meta<RecipeView> {
    using T = RecipeView;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value =
        object("name", &T::name, "ingredients", &T::ingredients);
};

}  // namespace glz

namespace {

[[nodiscard]] std::optional<std::string> FormatDate(
    const std::optional<std::chrono::sys_days>& date) {
    if (!date.has_value()) {
        return std::nullopt;
    }
    return std::format("{:%F}", date.value());
}

[[nodiscard]] ProductView ToProductView(const Product& product) {
    const auto& dates = product.GetDates();
    return ProductView{.name = product.GetName(),
                       .amount = product.GetAmount(),
                       .dimension = std::string(product.GetDimensionInString()),
                       .manufacture = FormatDate(dates.manufacture),
                       .expiration = FormatDate(dates.expiration)};
}

[[nodiscard]] std::vector<ProductView> BuildProductViews(
    std::span<const Product> products) {
    std::vector<ProductView> result;
    result.reserve(products.size());
    for (const auto& product : products) {
        result.push_back(ToProductView(product));
    }
    return result;
}

[[nodiscard]] std::vector<RecipeView> BuildRecipeViews(
    std::span<const Recipe> recipes) {
    std::vector<RecipeView> result;
    result.reserve(recipes.size());

    for (const auto& recipe : recipes) {
        auto ingredients = BuildProductViews(recipe.GetIngredients());
        result.push_back(RecipeView{.name = recipe.GetName(),
                                    .ingredients = std::move(ingredients)});
    }

    return result;
}

template <class Value>
std::expected<void, std::error_code> WriteYamlPayload(
    const Value& value, const fs::path& out_path) {
    std::string buffer;
    if (const auto ec = glz::write_yaml(value, buffer); ec) {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }

    const auto parent_path = out_path.parent_path();
    if (!parent_path.empty()) {
        std::error_code ec;
        fs::create_directories(parent_path, ec);
        if (ec) {
            return std::unexpected(std::error_code(ec.value(), ec.category()));
        }
    }

    std::ofstream out{out_path, std::ios::out | std::ios::trunc};
    if (!out.is_open()) {
        return std::unexpected(
            std::make_error_code(std::errc::no_such_file_or_directory));
    }

    out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if (!out.good()) {
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }

    return {};
}

}  // namespace

namespace io::yaml {

std::expected<void, std::error_code> WriteProductsYaml(
    std::span<const Product> products, const fs::path& out_path) {
    if (out_path.empty()) {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }
    return WriteYamlPayload(BuildProductViews(products), out_path);
}

std::expected<void, std::error_code> WriteRecipesYaml(
    std::span<const Recipe> recipes, const fs::path& out_path) {
    if (out_path.empty()) {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }
    return WriteYamlPayload(BuildRecipeViews(recipes), out_path);
}

}  // namespace io::yaml
