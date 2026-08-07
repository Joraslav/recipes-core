#include "IO.hpp"

#include "Args.hpp"
#include "io/console/Report.hpp"
#include "io/json/Report.hpp"
#include "io/yaml/Report.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <ostream>
#include <span>
#include <system_error>

using types::Recipe;
using namespace io::json;
using namespace io::yaml;
using namespace io::cli;

namespace io {

std::expected<void, std::error_code> RunReports(std::span<const Recipe> recipes,
                                                const ArgsOut& args,
                                                std::ostream& out) {
    if (!args.write_console && !args.write_json && !args.write_yaml) {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }

    if (args.write_json) {
        auto json_result = WriteRecipesJson(recipes, args.json_out_path);
        if (!json_result.has_value()) {
            return std::unexpected(json_result.error());
        }
    }

    if (args.write_yaml) {
        auto yaml_result = WriteRecipesYaml(recipes, args.yaml_out_path);
        if (!yaml_result.has_value()) {
            return std::unexpected(yaml_result.error());
        }
    }

    if (args.write_console) {
        PrintRecipes(recipes, args.is_full_info, out);
    }

    return {};
}

}  // namespace io
