#include "gtest/gtest.h"

#include "io/json/Report.hpp"
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

using namespace io::json;
using types::Dates;
using types::Dimension;
using types::Product;
using types::Recipe;
namespace fs = std::filesystem;

class TestIOJson : public ::testing::Test {
 protected:
    void SetUp() override {
        const auto unique_stamp = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        temp_dir_ =
            fs::temp_directory_path() / ("recipes_io_json_" + unique_stamp);
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

TEST_F(TestIOJson, WriteProductsJson_EmptyPath_ReturnsInvalidArgument) {
    const std::vector<Product> products{
        MakeProduct("Milk", 2, Dimension::LITER)};

    const auto result = WriteProductsJson(products);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(),
              std::make_error_code(std::errc::invalid_argument));
}

TEST_F(TestIOJson, WriteProductsJson_ValidPath_WritesJsonFile) {
    const auto manufacture = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{1} / std::chrono::day{2}};
    const auto expiration = std::chrono::sys_days{
        std::chrono::year{2026} / std::chrono::month{2} / std::chrono::day{3}};
    const std::vector<Product> products{MakeProduct(
        "Milk", 2, Dimension::LITER,
        Dates{.manufacture = manufacture, .expiration = expiration})};
    const fs::path out_path = TempDir() / "products.json";

    const auto result = WriteProductsJson(products, out_path);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_TRUE(fs::exists(out_path));

    const std::string json = ReadAll(out_path);
    EXPECT_NE(json.find("\"name\":\"Milk\""), std::string::npos);
    EXPECT_NE(json.find("\"amount\":2"), std::string::npos);
    EXPECT_NE(json.find("\"dimension\":\"l\""), std::string::npos);
    EXPECT_NE(json.find("\"manufacture\":\"2026-01-02\""), std::string::npos);
    EXPECT_NE(json.find("\"expiration\":\"2026-02-03\""), std::string::npos);
}

TEST_F(TestIOJson, WriteProductsJson_NestedPath_CreatesParentDirectories) {
    const std::vector<Product> products{
        MakeProduct("Salt", 10, Dimension::GRAMM),
    };
    const fs::path out_path = TempDir() / "nested" / "deep" / "products.json";

    const auto result = WriteProductsJson(products, out_path);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(fs::exists(out_path.parent_path()));
    EXPECT_TRUE(fs::exists(out_path));
}

TEST_F(TestIOJson, WriteRecipesJson_ValidPath_WritesJsonFileWithIngredients) {
    const std::vector<Recipe> recipes{
        MakeRecipe("Tea", {MakeProduct("Water", 200, Dimension::MILLILITER),
                           MakeProduct("Tea leaf", 5, Dimension::GRAMM)})};
    const fs::path out_path = TempDir() / "recipes.json";

    const auto result = WriteRecipesJson(recipes, out_path);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_TRUE(fs::exists(out_path));

    const std::string json = ReadAll(out_path);
    EXPECT_NE(json.find("\"name\":\"Tea\""), std::string::npos);
    EXPECT_NE(json.find("\"ingredients\":"), std::string::npos);
    EXPECT_NE(json.find("\"name\":\"Water\""), std::string::npos);
    EXPECT_NE(json.find("\"name\":\"Tea leaf\""), std::string::npos);
}

TEST_F(TestIOJson, WriteRecipesJson_EmptyPath_ReturnsInvalidArgument) {
    const std::vector<Recipe> recipes{MakeRecipe("Tea")};

    const auto result = WriteRecipesJson(recipes);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(),
              std::make_error_code(std::errc::invalid_argument));
}

TEST_F(TestIOJson, WriteProductsJson_PathIsDirectory_ReturnsError) {
    const std::vector<Product> products{
        MakeProduct("Salt", 10, Dimension::GRAMM),
    };
    const fs::path out_path = TempDir();

    const auto result = WriteProductsJson(products, out_path);
    EXPECT_FALSE(result.has_value());
}

TEST_F(TestIOJson, WriteProductsJson_SamePathTwice_SecondWriteOverwritesFile) {
    const fs::path out_path = TempDir() / "overwrite.json";
    const std::vector<Product> first_products{
        MakeProduct("Milk", 2, Dimension::LITER),
    };
    const std::vector<Product> second_products{
        MakeProduct("Flour", 500, Dimension::GRAMM),
    };

    ASSERT_TRUE(WriteProductsJson(first_products, out_path).has_value());
    ASSERT_TRUE(WriteProductsJson(second_products, out_path).has_value());

    const std::string json = ReadAll(out_path);
    EXPECT_NE(json.find("\"name\":\"Flour\""), std::string::npos);
    EXPECT_EQ(json.find("\"name\":\"Milk\""), std::string::npos);
}

}  // namespace
