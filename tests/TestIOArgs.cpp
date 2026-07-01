#include "gtest/gtest.h"

#include "io/Args.hpp"

#include <span>
#include <string>
#include <vector>

namespace {

using io::Args;
using io::ParseArgs;
using io::RecipeSelection;

class TestIOArgs : public ::testing::Test {
 protected:
    [[nodiscard]] static std::vector<const char*> BuildArgv(
        const std::vector<std::string>& values) {
        std::vector<const char*> argv;
        argv.reserve(values.size());
        for (const auto& value : values) {
            argv.push_back(value.c_str());
        }
        return argv;
    }
};

TEST_F(TestIOArgs, ParseArgs_EmptyInput_ReturnsDefaultConfig) {
    const std::vector<std::string> args{"recipes"};
    const auto argv = BuildArgv(args);

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    const Args& parsed = parse_result.value();
    EXPECT_FALSE(parsed.show_help);
    EXPECT_EQ(parsed.app.recipe_selection, RecipeSelection::ALL);
    EXPECT_TRUE(parsed.app.products.empty());
    EXPECT_TRUE(parsed.app.recipes.empty());
    EXPECT_TRUE(parsed.out.is_full_info);
    EXPECT_TRUE(parsed.out.write_console);
    EXPECT_TRUE(parsed.out.write_json);
    EXPECT_TRUE(parsed.out.write_yaml);
    EXPECT_EQ(parsed.out.json_out_path, io::GetProjectOutDir() / "info.json");
    EXPECT_EQ(parsed.out.yaml_out_path, io::GetProjectOutDir() / "info.yaml");
}

TEST_F(TestIOArgs, ParseArgs_HelpFlag_SetsShowHelp) {
    const std::vector<std::string> args{"recipes", "--help"};
    const auto argv = BuildArgv(args);

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    EXPECT_TRUE(parse_result->show_help);
}

TEST_F(TestIOArgs, ParseArgs_CustomOutputsAndQuery_AppliesFlagsAndPaths) {
    const std::vector<std::string> args{
        "recipes",    "--short",    "--no-console",
        "--cookable", "--db-path",  "tmp/recipes.db",
        "--json-out", "tmp/r.json", "--yaml-out=tmp/r.yaml"};
    const auto argv = BuildArgv(args);

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    const Args& parsed = parse_result.value();
    EXPECT_EQ(parsed.app.recipe_selection, RecipeSelection::COOKABLE);
    EXPECT_EQ(parsed.app.db_path, "tmp/recipes.db");
    EXPECT_FALSE(parsed.out.is_full_info);
    EXPECT_FALSE(parsed.out.write_console);
    EXPECT_TRUE(parsed.out.write_json);
    EXPECT_TRUE(parsed.out.write_yaml);
    EXPECT_EQ(parsed.out.json_out_path, "tmp/r.json");
    EXPECT_EQ(parsed.out.yaml_out_path, "tmp/r.yaml");
}

TEST_F(TestIOArgs, ParseArgs_MissingOptionValue_ReturnsError) {
    const std::vector<std::string> args{"recipes", "--json-out"};
    const auto argv = BuildArgv(args);

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_FALSE(parse_result.has_value());
    EXPECT_NE(parse_result.error().find("--json-out"), std::string::npos);
}

TEST_F(TestIOArgs, ParseArgs_UnknownFlag_ReturnsError) {
    const std::vector<std::string> args{"recipes", "--unknown"};
    const auto argv = BuildArgv(args);

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_FALSE(parse_result.has_value());
    EXPECT_NE(parse_result.error().find("Unknown argument"), std::string::npos);
}

}  // namespace
