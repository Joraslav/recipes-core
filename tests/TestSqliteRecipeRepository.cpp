#include "gtest/gtest.h"

#include "app/RecipeService.hpp"
#include "app/SqliteRecipeRepository.hpp"
#include "db/DBManager.hpp"
#include "types/kitchen/Types.hpp"

using app::RecipeService;
using app::SqliteRecipeRepository;
using db::DBManager;
using types::Dimension;
using types::Product;

namespace {

class TestSqliteRecipeRepository : public ::testing::Test {
 protected:
    DBManager db_manager;
    SqliteRecipeRepository repository{db_manager};
    RecipeService service{repository};
};

TEST_F(TestSqliteRecipeRepository,
       ProductCrud_ValidProduct_PersistsThroughService) {
    const auto created_product =
        service.CreateProduct(Product{"Egg", 4, Dimension::Piece});

    ASSERT_TRUE(created_product.has_value())
        << created_product.error().GetMessage();
    ASSERT_TRUE(created_product->GetId().has_value());

    const auto fetched_product =
        service.GetProduct(created_product->GetId().value());
    ASSERT_TRUE(fetched_product.has_value())
        << fetched_product.error().GetMessage();
    ASSERT_TRUE(fetched_product->has_value());
    EXPECT_EQ(fetched_product->value().GetName(), "Egg");

    ASSERT_TRUE(service
                    .UpdateProduct(created_product->GetId().value(),
                                   Product{"Egg", 10, Dimension::Piece})
                    .has_value());
    ASSERT_TRUE(
        service.DeleteProduct(created_product->GetId().value()).has_value());
    EXPECT_FALSE(service.GetProduct(created_product->GetId().value())
                     .value()
                     .has_value());
}

}  // namespace