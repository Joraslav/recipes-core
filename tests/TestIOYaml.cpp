#include "gtest/gtest.h"

#include "io/yaml/Report.hpp"
#include "types/kitchen/Types.hpp"

#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using namespace io::yaml;
using types::Dates;
using types::Dimension;
using types::Product;
using types::Recipe;
namespace fs = std::filesystem;

class TestIOYaml : public ::testing::Test {
 protected:
    void SetUp() override {
        const auto unique_stamp = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        temp_dir_ =
            fs::temp_directory_path() / ("recipes_io_yaml_" + unique_stamp);
        std::error_code ec;
        fs::create_directories(temp_dir_, ec);
        ASSERT_FALSE(ec) << "Failed to create temp dir: " << ec.message();
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(temp_dir_, ec);
    }

    [[nodiscard]] static Product MakeProduct(std::string_view name, int amount,
                                             Dimension dimension,
                                             Dates dates = {}) {
        return Product{name, amount, dimension, dates};
    }

    [[nodiscard]] static Recipe MakeRecipe(
        std::string_view name,
        std::initializer_list<Product> ingredients = {}) {
        return Recipe{name, ingredients};
    }

    [[nodiscard]] static std::string ReadAll(const fs::path& path) {
        std::ifstream in{path};
        if (!in.is_open()) {
            return {};
        }
        std::ostringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    [[nodiscard]] const fs::path& TempDir() const noexcept { return temp_dir_; }

 private:
    fs::path temp_dir_;
};

TEST_F(TestIOYaml, WriteProductsYaml_EmptyPath_ReturnsInvalidArgument) {
    const std::vector<Product> products{
        MakeProduct("Milk", 2, Dimension::LITER)};

    const auto result = WriteProductsYaml(products);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(),
              std::make_error_code(std::errc::invalid_argument));
}

TEST_F(TestIOYaml, WriteProductsYaml_ValidPath_WritesYamlFile) {
    const auto manufacture = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{1} / std::chrono::day{2}};
    const auto expiration = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{2} / std::chrono::day{3}};
    const std::vector<Product> products{MakeProduct(
        "Milk", 2, Dimension::LITER,
        Dates{.manufacture = manufacture, .expiration = expiration})};
    const fs::path out_path = TempDir() / "products.yaml";

    const auto result = WriteProductsYaml(products, out_path);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_TRUE(fs::exists(out_path));

    const std::string yaml = ReadAll(out_path);
    EXPECT_NE(yaml.find("name: Milk"), std::string::npos);
    EXPECT_NE(yaml.find("amount: 2"), std::string::npos);
    EXPECT_NE(yaml.find("dimension: l"), std::string::npos);
    EXPECT_NE(yaml.find("manufacture:"), std::string::npos);
    EXPECT_NE(yaml.find("2026-01-02"), std::string::npos);
    EXPECT_NE(yaml.find("expiration:"), std::string::npos);
    EXPECT_NE(yaml.find("2026-02-03"), std::string::npos);
}

TEST_F(TestIOYaml, WriteProductsYaml_NestedPath_CreatesParentDirectories) {
    const std::vector<Product> products{
        MakeProduct("Salt", 10, Dimension::GRAMM),
    };
    const fs::path out_path = TempDir() / "nested" / "deep" / "products.yaml";

    const auto result = WriteProductsYaml(products, out_path);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(fs::exists(out_path.parent_path()));
    EXPECT_TRUE(fs::exists(out_path));
}

TEST_F(TestIOYaml, WriteRecipesYaml_ValidPath_WritesYamlFileWithIngredients) {
    const std::vector<Recipe> recipes{
        MakeRecipe("Tea", {MakeProduct("Water", 200, Dimension::MILLILITER),
                           MakeProduct("Tea leaf", 5, Dimension::GRAMM)})};
    const fs::path out_path = TempDir() / "recipes.yaml";

    const auto result = WriteRecipesYaml(recipes, out_path);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_TRUE(fs::exists(out_path));

    const std::string yaml = ReadAll(out_path);
    EXPECT_NE(yaml.find("name: Tea"), std::string::npos);
    EXPECT_NE(yaml.find("ingredients:"), std::string::npos);
    EXPECT_NE(yaml.find("name: Water"), std::string::npos);
    EXPECT_NE(yaml.find("name: Tea leaf"), std::string::npos);
}

TEST_F(TestIOYaml, WriteRecipesYaml_EmptyPath_ReturnsInvalidArgument) {
    const std::vector<Recipe> recipes{MakeRecipe("Tea")};

    const auto result = WriteRecipesYaml(recipes);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(),
              std::make_error_code(std::errc::invalid_argument));
}

TEST_F(TestIOYaml, WriteProductsYaml_PathIsDirectory_ReturnsError) {
    const std::vector<Product> products{
        MakeProduct("Salt", 10, Dimension::GRAMM),
    };
    const fs::path out_path = TempDir();

    const auto result = WriteProductsYaml(products, out_path);
    EXPECT_FALSE(result.has_value());
}

TEST_F(TestIOYaml, WriteProductsYaml_SamePathTwice_SecondWriteOverwritesFile) {
    const fs::path out_path = TempDir() / "overwrite.yaml";
    const std::vector<Product> first_products{
        MakeProduct("Milk", 2, Dimension::LITER),
    };
    const std::vector<Product> second_products{
        MakeProduct("Flour", 500, Dimension::GRAMM),
    };

    ASSERT_TRUE(WriteProductsYaml(first_products, out_path).has_value());
    ASSERT_TRUE(WriteProductsYaml(second_products, out_path).has_value());

    const std::string yaml = ReadAll(out_path);
    EXPECT_NE(yaml.find("name: Flour"), std::string::npos);
    EXPECT_EQ(yaml.find("name: Milk"), std::string::npos);
}

}  // namespace