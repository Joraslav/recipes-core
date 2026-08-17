#include "IO.hpp"

#include "args/Args.hpp"
#include "concepts/Concepts.hpp"
#include "io/console/Report.hpp"
#include "io/json/Report.hpp"
#include "io/yaml/Report.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <iostream>
#include <ostream>
#include <span>
#include <system_error>

using types::Product;
using types::Recipe;
using namespace io::json;
using namespace io::yaml;
using namespace io::cli;
using concepts::ProductOrRecipe;
using namespace io::arg;
using ArgsOut = io::arg::Args::ArgsOut;

namespace {

template <ProductOrRecipe Tv>
std::expected<void, std::error_code> WriteReports(std::span<const Tv> items,
                                                  const ArgsOut& args,
                                                  std::ostream& out) {
    if (args.write_console) {
        PrintItems(items, args.is_full_info, out);
    }
    if (args.write_json) {
        auto json_result = WriteItemsJson(items, args.json_out_path);
        if (!json_result.has_value()) {
            return std::unexpected(json_result.error());
        }
    }
    if (args.write_yaml) {
        auto yaml_result = WriteItemsYaml(items, args.yaml_out_path);
        if (!yaml_result.has_value()) {
            return std::unexpected(yaml_result.error());
        }
    }

    return {};
}

}  // namespace

namespace io {

std::expected<void, std::error_code> ReportRecipes(
    std::span<const Recipe> recipes, const Args::ArgsOut& args,
    std::ostream& out) {
    if (!args.write_console && !args.write_json && !args.write_yaml) {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }

    return WriteReports(recipes, args, out);
}

std::expected<void, std::error_code> ReportProducts(
    std::span<const Product> products, const Args::ArgsOut& args,
    std::ostream& out) {
    if (!args.write_console && !args.write_json && !args.write_yaml) {
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }

    return WriteReports(products, args, out);
}

}  // namespace io
