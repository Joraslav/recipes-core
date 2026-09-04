#include "Json.hpp"

#include "api/Types.hpp"

#include <glaze/core/common.hpp>
#include <glaze/forward.hpp>
#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

#include <expected>
#include <string>
#include <string_view>

using api::ErrorResponse;
using api::ProductRequest;
using api::ProductResponse;
using api::RecipeRequest;
using api::RecipeResponse;

namespace glz {

template <>
struct meta<ProductRequest> {
    using T = ProductRequest;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value = object(
        "name", &T::name, "amount", &T::amount, "dimension", &T::dimension,
        "manufacture", &T::manufacture, "expiration", &T::expiration);
};

template <>
struct meta<ProductResponse> {
    using T = ProductResponse;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value =
        object("id", &T::id, "name", &T::name, "amount", &T::amount,
               "dimension", &T::dimension, "manufacture", &T::manufacture,
               "expiration", &T::expiration);
};

template <>
struct meta<RecipeRequest> {
    using T = RecipeRequest;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value =
        object("name", &T::name, "ingredients", &T::ingredients);
};

template <>
struct meta<RecipeResponse> {
    using T = RecipeResponse;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value =
        object("id", &T::id, "name", &T::name, "ingredients", &T::ingredients);
};

template <>
struct meta<ErrorResponse> {
    using T = ErrorResponse;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value =
        object("code", &T::code, "message", &T::message);
};

}  // namespace glz

namespace {

template <typename T>
std::expected<T, std::string> ParseJson(std::string_view json) {
    T value;
    if (const auto error = glz::read_json(value, json); error) {
        return std::unexpected(glz::format_error(error, json));
    }
    return value;
}

template <typename T>
std::expected<std::string, std::string> WriteJson(const T& value) {
    std::string json;
    if (const auto error = glz::write_json(value, json); error) {
        return std::unexpected(glz::format_error(error, json));
    }
    return json;
}

}  // namespace

namespace api {

std::expected<ProductRequest, std::string> ParseProductRequest(
    std::string_view json) {
    return ParseJson<ProductRequest>(json);
}

std::expected<RecipeRequest, std::string> ParseRecipeRequest(
    std::string_view json) {
    return ParseJson<RecipeRequest>(json);
}

std::expected<std::string, std::string> SerializeJson(
    const ProductResponse& response) {
    return WriteJson(response);
}

std::expected<std::string, std::string> SerializeJson(
    const RecipeResponse& response) {
    return WriteJson(response);
}

std::expected<std::string, std::string> SerializeJson(
    const ErrorResponse& response) {
    return WriteJson(response);
}

}  // namespace api