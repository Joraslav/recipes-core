#include "IO.hpp"

#include "io/console/Report.hpp"
#include "io/json/Report.hpp"
#include "io/yaml/Report.hpp"

#include <expected>
#include <exception>
#include <format>
#include <string>

namespace io {

std::expected<void, std::string> RunReports(std::span<const types::Recipe> recipes,
											const Args& args,
											std::ostream& out) {
	if (!args.write_console && !args.write_json && !args.write_yaml) {
		return std::unexpected(
			"No output selected. Enable at least one of console/json/yaml outputs.");
	}

	try {
		if (args.write_json) {
			json::WriteRecipesJson(recipes, args.json_out_path);
		}

		if (args.write_yaml) {
			yaml::WriteRecipesYaml(recipes, args.yaml_out_path);
		}

		if (args.write_console) {
			cli::PrintRecipes(recipes, args.is_full_info, out);
		}
	} catch (const std::exception& ex) {
		return std::unexpected(std::format("Failed to produce reports: {}", ex.what()));
	}

	return {};
}

}  // namespace io
