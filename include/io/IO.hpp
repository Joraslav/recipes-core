#pragma once

#include "concepts/Concepts.hpp"
#include "io/args/Args.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <iostream>
#include <ostream>
#include <span>
#include <system_error>

namespace io {

[[nodiscard]] std::expected<void, std::error_code> ReportRecipes(
    std::span<const types::Recipe> recipes, const arg::Args::ArgsOut& args,
    std::ostream& out = std::cout);

[[nodiscard]] std::expected<void, std::error_code> ReportProducts(
    std::span<const types::Product>, const arg::Args::ArgsOut& args,
    std::ostream& = std::cout);

template <concepts::ProductOrRecipe Tv>
[[nodiscard]] std::expected<void, std::error_code> ReportsItems(
    std::span<const Tv> items, const arg::Args::ArgsOut& args,
    std::ostream& out = std::cout) {
    if constexpr (std::is_same_v<Tv, types::Product>) {
        return ReportProducts(items, args, out);
    } else if constexpr (std::is_same_v<Tv, types::Recipe>) {
        return ReportRecipes(items, args, out);
    } else {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }
}

}  // namespace io
