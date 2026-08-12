#pragma once

#include "io/args/Args.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <iostream>
#include <ostream>
#include <span>
#include <system_error>

namespace io {

[[nodiscard]] std::expected<void, std::error_code> RunReports(
    std::span<const types::Recipe> recipes, const ArgsOut& args,
    std::ostream& out = std::cout);

}  // namespace io
