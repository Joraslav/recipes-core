#pragma once

#include "io/args/Args.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <iosfwd>
#include <system_error>
#include <vector>

namespace app {

[[nodiscard]] std::expected<std::vector<types::Recipe>, std::error_code>
Execute(const io::arg::Args::AppArgs& args);

[[nodiscard]] std::expected<void, std::error_code> Run(
    const io::arg::Args& args);

[[nodiscard]] std::expected<void, std::error_code> Run(
    const io::arg::Args& args, std::ostream& out);

}  // namespace app
