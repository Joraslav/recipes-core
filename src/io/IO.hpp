#pragma once

#include "io/Args.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <iostream>
#include <ostream>
#include <span>

namespace io {

[[nodiscard]] std::expected<void, std::string> RunReports(
	std::span<const types::Recipe> recipes, const Args& args,
	std::ostream& out = std::cout);

}  // namespace io
