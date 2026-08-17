#include "gtest/gtest.h"

#include "io/args/Args.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using io::arg::Args;
using io::arg::BuildUsage;
using io::arg::GetProjectDataDir;
using io::arg::GetProjectOutDir;
using io::arg::HasAnyOutput;
using io::arg::ParseArgs;
using types::RecipeSelection;

class TestIOArgs : public ::testing::Test {
 protected:
    static std::vector<std::string_view> BuildArgv(
        const std::vector<std::string>& values) {
        std::vector<std::string_view> argv;
        argv.reserve(values.size());
        for (const auto& value : values) {
            argv.push_back(value);
        }
        return argv;
    }
};

TEST_F(TestIOArgs, ParseArgs_EmptyInput_ReturnsDefaultConfig) {
    const std::vector<std::string> args{"recipes"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    const Args& parsed = parse_result.value();
    EXPECT_FALSE(parsed.ShowHelp());
    EXPECT_EQ(parsed.App().recipe_selection, RecipeSelection::All);
    EXPECT_EQ(parsed.App().db_path, GetProjectDataDir() / "table.db");
    EXPECT_TRUE(parsed.App().products.empty());
    EXPECT_TRUE(parsed.App().recipes.empty());
    EXPECT_TRUE(parsed.Out().is_full_info);
    EXPECT_TRUE(parsed.Out().write_console);
    EXPECT_TRUE(parsed.Out().write_json);
    EXPECT_TRUE(parsed.Out().write_yaml);
    EXPECT_EQ(parsed.Out().json_out_path, GetProjectOutDir() / "info.json");
    EXPECT_EQ(parsed.Out().yaml_out_path, GetProjectOutDir() / "info.yaml");
}

TEST_F(TestIOArgs, ParseArgs_HelpFlag_SetsShowHelp) {
    const std::vector<std::string> args{"recipes", "--help"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    EXPECT_TRUE(parse_result->ShowHelp());
}

TEST_F(TestIOArgs, ParseArgs_OutputFlags_ApplyLastValueWins) {
    const std::vector<std::string> args{
        "recipes",   "--short", "--full-info", "--no-console", "--console",
        "--no-json", "--json",  "--no-yaml",   "--yaml"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    const Args& parsed = parse_result.value();
    EXPECT_TRUE(parsed.Out().is_full_info);
    EXPECT_TRUE(parsed.Out().write_console);
    EXPECT_TRUE(parsed.Out().write_json);
    EXPECT_TRUE(parsed.Out().write_yaml);
    EXPECT_TRUE(HasAnyOutput(parsed.Out()));
}

TEST_F(TestIOArgs, ParseArgs_SelectionAndPaths_SetExpectedValues) {
    const std::vector<std::string> args{
        "recipes", "--cookable", "--db-path=tmp/recipes.db",
        "--json-out=tmp/r.json", "--yaml-out=tmp/r.yaml"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    const Args& parsed = parse_result.value();
    EXPECT_EQ(parsed.App().recipe_selection, RecipeSelection::Cookable);
    EXPECT_EQ(parsed.App().db_path, "tmp/recipes.db");
    EXPECT_EQ(parsed.Out().json_out_path, "tmp/r.json");
    EXPECT_EQ(parsed.Out().yaml_out_path, "tmp/r.yaml");
}

TEST_F(TestIOArgs, ParseArgs_AllRecipes_OverridesCookable) {
    const std::vector<std::string> args{"recipes", "--cookable",
                                        "--all-recipes"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    EXPECT_EQ(parse_result->App().recipe_selection, RecipeSelection::All);
}

TEST_F(TestIOArgs, ParseArgs_MissingOptionValue_ReturnsError) {
    const std::vector<std::string> args{"recipes", "--json-out"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_FALSE(parse_result.has_value());
    EXPECT_NE(parse_result.error().find("--json-out"), std::string::npos);
}

TEST_F(TestIOArgs, ParseArgs_EmptyValueForOutputFlag_ReturnsError) {
    const std::vector<std::string> args{"recipes", "--yaml-out="};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_FALSE(parse_result.has_value());
    EXPECT_NE(parse_result.error().find("--yaml-out"), std::string::npos);
}

TEST_F(TestIOArgs, ParseArgs_UnknownFlag_ReturnsError) {
    const std::vector<std::string> args{"recipes", "--unknown"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_FALSE(parse_result.has_value());
    EXPECT_NE(parse_result.error().find("Unknown argument"), std::string::npos);
}

TEST_F(TestIOArgs, BuildUsage_ContainsUpdatedFlags) {
    const auto usage = BuildUsage("recipes");

    EXPECT_NE(usage.find("Usage: recipes [options]"), std::string::npos);
    EXPECT_NE(usage.find("--all-recipes"), std::string::npos);
    EXPECT_NE(usage.find("--cookable"), std::string::npos);
    EXPECT_NE(usage.find("--json-out=<path>"), std::string::npos);
    EXPECT_NE(usage.find("--db-path=<path>"), std::string::npos);
}

TEST_F(TestIOArgs, ParseArgs_EqualJsonAndYamlOutputPaths_ReturnsError) {
    const std::vector<std::string> args{"recipes", "--json-out=tmp/out.json",
                                        "--yaml-out=tmp/out.json"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_FALSE(parse_result.has_value());
    EXPECT_NE(parse_result.error().find(
                  "JSON and YAML output paths must be different"),
              std::string::npos);
}

TEST_F(TestIOArgs, ParseArgs_LastValueWinsForOutputPaths) {
    const std::vector<std::string> args{
        "recipes", "--json-out=tmp/first.json", "--json-out=tmp/second.json",
        "--yaml-out=tmp/first.yaml", "--yaml-out=tmp/second.yaml"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    EXPECT_EQ(parse_result->Out().json_out_path, "tmp/second.json");
    EXPECT_EQ(parse_result->Out().yaml_out_path, "tmp/second.yaml");
}

TEST_F(TestIOArgs, ParseArgs_DifferentOutputPaths_Succeeds) {
    const std::vector<std::string> args{"recipes", "--json-out=tmp/report.json",
                                        "--yaml-out=tmp/report.yaml"};
    const auto argv = BuildArgv(args);

    const auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    EXPECT_EQ(parse_result->Out().json_out_path, "tmp/report.json");
    EXPECT_EQ(parse_result->Out().yaml_out_path, "tmp/report.yaml");
}

}  // namespace
