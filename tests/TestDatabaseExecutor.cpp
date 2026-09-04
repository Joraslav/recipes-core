#include "gtest/gtest.h"

#include "app/DatabaseExecutor.hpp"
#include "app/Error.hpp"
#include "app/RecipeService.hpp"
#include "types/kitchen/Types.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <chrono>
#include <expected>
#include <future>
#include <memory>
#include <thread>
#include <utility>

namespace asio = boost::asio;

namespace {

using app::DatabaseExecutor;
using app::Error;
using app::ErrorCode;
using app::RecipeService;
using types::Dimension;
using types::Product;

class TestDatabaseExecutor : public ::testing::Test {
 protected:
    DatabaseExecutor executor{":memory:", 1};
};

TEST_F(TestDatabaseExecutor, Submit_CreateProduct_PersistsOnWorker) {
    auto created_product = executor.Submit([](RecipeService& service) {
        return service.CreateProduct(Product{"Egg", 4, Dimension::Piece});
    });

    const auto result = created_product.get();

    ASSERT_TRUE(result.has_value()) << result.error().GetMessage();
    ASSERT_TRUE(result->GetId().has_value());
    EXPECT_EQ(result->GetName(), "Egg");
}

TEST_F(TestDatabaseExecutor, Submit_QueueFull_ReturnsOverloadedError) {
    auto worker_started = std::make_shared<std::promise<void>>();
    auto release_worker = std::make_shared<std::promise<void>>();
    auto release_future =
        std::make_shared<std::future<void>>(release_worker->get_future());

    auto running_operation =
        executor.Submit([worker_started, release_future](
                            RecipeService&) -> std::expected<int, Error> {
            worker_started->set_value();
            release_future->wait();
            return 1;
        });
    ASSERT_EQ(worker_started->get_future().wait_for(std::chrono::seconds(1)),
              std::future_status::ready);

    auto queued_operation = executor.Submit(
        [](RecipeService&) -> std::expected<int, Error> { return 2; });
    auto overloaded_operation = executor.Submit(
        [](RecipeService&) -> std::expected<int, Error> { return 3; });

    const auto overloaded_result = overloaded_operation.get();
    ASSERT_FALSE(overloaded_result.has_value());
    EXPECT_EQ(overloaded_result.error().GetCode(), ErrorCode::Overloaded);

    release_worker->set_value();
    ASSERT_TRUE(running_operation.get().has_value());
    ASSERT_TRUE(queued_operation.get().has_value());
}

TEST_F(TestDatabaseExecutor,
       AsyncSubmit_CreateProduct_CompletesOnAssociatedExecutor) {
    asio::io_context io_context;
    auto work_guard = asio::make_work_guard(io_context);
    std::jthread io_worker([&io_context] { io_context.run(); });
    std::promise<std::expected<Product, Error>> completion;
    auto completion_result = completion.get_future();

    executor.AsyncSubmit(
        [](RecipeService& service) {
            return service.CreateProduct(Product{"Milk", 1, Dimension::Liter});
        },
        asio::bind_executor(
            io_context.get_executor(),
            [&completion](std::expected<Product, Error> result) {
                completion.set_value(std::move(result));
            }));

    const auto result = completion_result.get();
    work_guard.reset();

    ASSERT_TRUE(result.has_value()) << result.error().GetMessage();
    EXPECT_EQ(result->GetName(), "Milk");
}

}  // namespace