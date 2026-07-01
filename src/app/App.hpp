#pragma once

#include "io/Args.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <iosfwd>
#include <string>
#include <vector>

namespace app {

[[nodiscard]] std::expected<std::vector<types::Recipe>, std::string> Execute(
    const io::AppArgs& args);

[[nodiscard]] std::expected<void, std::string> Run(const io::Args& args);

[[nodiscard]] std::expected<void, std::string> Run(const io::Args& args,
                                                   std::ostream& out);

}  // namespace app
