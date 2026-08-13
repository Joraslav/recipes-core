#include "gtest/gtest.h"

#include "io/args/Args.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using io::Args;
using io::ParseArgs;
using io::RecipeSelection;

class TestIOArgs : public ::testing::Test {
 protected:
    [[nodiscard]] static std::vector<std::string_view> BuildArgv(
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

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    const Args& parsed = parse_result.value();
    EXPECT_FALSE(parsed.ShowHelp());
    EXPECT_EQ(parsed.App().recipe_selection, RecipeSelection::All);
    EXPECT_TRUE(parsed.App().products.empty());
    EXPECT_TRUE(parsed.App().recipes.empty());
    EXPECT_TRUE(parsed.Out().is_full_info);
    EXPECT_TRUE(parsed.Out().write_console);
    EXPECT_TRUE(parsed.Out().write_json);
    EXPECT_TRUE(parsed.Out().write_yaml);
    EXPECT_EQ(parsed.Out().json_out_path, io::GetProjectOutDir() / "info.json");
    EXPECT_EQ(parsed.Out().yaml_out_path, io::GetProjectOutDir() / "info.yaml");
}

TEST_F(TestIOArgs, ParseArgs_HelpFlag_SetsShowHelp) {
    const std::vector<std::string> args{"recipes", "--help"};
    const auto argv = BuildArgv(args);

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    EXPECT_TRUE(parse_result->ShowHelp());
}

TEST_F(TestIOArgs, ParseArgs_CustomOutputsAndQuery_AppliesFlagsAndPaths) {
    const std::vector<std::string> args{"recipes",
                                        "--short",
                                        "--no-console",
                                        "--cookable",
                                        "--db-path=tmp/recipes.db",
                                        "--json-out=tmp/r.json",
                                        "--yaml-out=tmp/r.yaml"};
    const auto argv = BuildArgv(args);

    auto parse_result = ParseArgs(std::span(argv));

    ASSERT_TRUE(parse_result.has_value());
    const Args& parsed = parse_result.value();
    EXPECT_EQ(parsed.App().recipe_selection, RecipeSelection::Cookable);
    EXPECT_EQ(parsed.App().db_path, "tmp/recipes.db");
    EXPECT_FALSE(parsed.Out().is_full_info);
    EXPECT_FALSE(parsed.Out().write_console);
    EXPECT_TRUE(parsed.Out().write_json);
    EXPECT_TRUE(parsed.Out().write_yaml);
    EXPECT_EQ(parsed.Out().json_out_path, "tmp/r.json");
    EXPECT_EQ(parsed.Out().yaml_out_path, "tmp/r.yaml");
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
