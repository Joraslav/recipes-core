#include "gtest/gtest.h"

#include "api/Json.hpp"
#include "api/Types.hpp"

#include <optional>
#include <string>

using api::ParseRecipeRequest;
using api::RecipeResponse;

namespace {

class TestApi : public ::testing::Test {};

TEST_F(TestApi, ParseProductRequest_ValidJson_ReturnsRequest) {
    const auto result = api::ParseProductRequest(
        R"({"name":"Milk","amount":1000,"dimension":"ml","manufacture":"2026-01-02","expiration":null})");

    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result->name, "Milk");
    EXPECT_EQ(result->amount, 1000);
    EXPECT_EQ(result->dimension, "ml");
    ASSERT_TRUE(result->manufacture.has_value());
    EXPECT_EQ(result->manufacture.value(), "2026-01-02");
    EXPECT_FALSE(result->expiration.has_value());
}

TEST_F(TestApi, ParseRecipeRequest_InvalidJson_ReturnsError) {
    const auto result = ParseRecipeRequest("{invalid}");

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(TestApi, SerializeJson_RecipeResponse_ContainsContractFields) {
    const RecipeResponse recipe{
        .id = 42,
        .name = "Tea",
        .ingredients = {{.id = 7,
                         .name = "Water",
                         .amount = 200,
                         .dimension = "ml",
                         .manufacture = std::nullopt,
                         .expiration = std::nullopt}},
    };

    const auto result = api::SerializeJson(recipe);

    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(
        *result,
        R"({"id":42,"name":"Tea","ingredients":[{"id":7,"name":"Water","amount":200,"dimension":"ml"}]})");
}

}  // namespace