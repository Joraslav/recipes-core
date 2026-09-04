#include "gtest/gtest.h"

#include "app/DatabaseExecutor.hpp"
#include "app/Error.hpp"
#include "app/RecipeService.hpp"
#include "server/Router.hpp"
#include "types/kitchen/Types.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/cobalt/spawn.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <future>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

using app::DatabaseExecutor;
using app::Error;
using app::RecipeService;
using types::Product;
using types::Recipe;
namespace http = net::http;

namespace {

class TestHttpRouter : public ::testing::Test {};

TEST_F(TestHttpRouter, Handle_HealthGet_ReturnsOkJsonResponse) {
    const http::request<http::string_body> request{http::verb::get, "/healthz",
                                                   11};

    const auto response = net::Router::Handle(request);

    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_EQ(response[http::field::content_type], "application/json");
    EXPECT_EQ(response.body(), "{\"status\":\"ok\"}");
}

TEST_F(TestHttpRouter, Handle_HealthPost_ReturnsMethodNotAllowed) {
    const http::request<http::string_body> request{http::verb::post, "/healthz",
                                                   11};

    const auto response = net::Router::Handle(request);

    EXPECT_EQ(response.result(), http::status::method_not_allowed);
    EXPECT_NE(response.body().find("method_not_allowed"), std::string::npos);
}

TEST_F(TestHttpRouter, Handle_UnknownTarget_ReturnsNotFound) {
    const http::request<http::string_body> request{http::verb::get,
                                                   "/v1/products", 11};

    const auto response = net::Router::Handle(request);

    EXPECT_EQ(response.result(), http::status::not_found);
    EXPECT_NE(response.body().find("not_found"), std::string::npos);
}

TEST_F(TestHttpRouter, HandleAsync_ProductsGet_ReturnsPersistedProducts) {
    DatabaseExecutor database_executor{":memory:", 4};
    auto created_product = database_executor.Submit([](RecipeService& service) {
        return service.CreateProduct(
            Product{"Egg", 4, types::Dimension::Piece});
    });
    ASSERT_TRUE(created_product.get().has_value());

    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);
    std::jthread io_worker([&io_context] { io_context.run(); });
    net::Router router{&database_executor};
    std::promise<http::response<http::string_body>> completion;
    auto response_future = completion.get_future();
    const http::request<http::string_body> request{http::verb::get,
                                                   "/v1/products", 11};

    boost::cobalt::spawn(
        io_context, router.HandleAsync(request),
        [&completion](
            std::exception_ptr
                exception,  // NOLINT (performance-unnecessary-value-param)
            http::response<http::string_body> response) {
            if (exception) {
                completion.set_exception(exception);
                return;
            }
            completion.set_value(std::move(response));
        });

    const auto response = response_future.get();
    work_guard.reset();

    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_NE(response.body().find("Egg"), std::string::npos);
}

TEST_F(TestHttpRouter, HandleAsync_ProductPost_ReturnsCreatedProduct) {
    DatabaseExecutor database_executor{":memory:", 4};
    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);
    std::jthread io_worker([&io_context] { io_context.run(); });
    net::Router router{&database_executor};
    std::promise<http::response<http::string_body>> completion;
    auto response_future = completion.get_future();
    http::request<http::string_body> request{http::verb::post, "/v1/products",
                                             11};
    request.body() = R"({"name":"Milk","amount":1,"dimension":"l"})";

    boost::cobalt::spawn(
        io_context, router.HandleAsync(std::move(request)),
        [&completion](
            std::exception_ptr
                exception,  // NOLINT (performance-unnecessary-value-param)
            http::response<http::string_body> response) {
            if (exception) {
                completion.set_exception(exception);
                return;
            }
            completion.set_value(std::move(response));
        });

    const auto response = response_future.get();
    work_guard.reset();

    EXPECT_EQ(response.result(), http::status::created);
    EXPECT_NE(response.body().find("Milk"), std::string::npos);
    EXPECT_NE(response.body().find("\"id\":"), std::string::npos);
}

TEST_F(TestHttpRouter, HandleAsync_ProductCrudById_CompletesLifecycle) {
    DatabaseExecutor database_executor{":memory:", 4};
    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);
    std::jthread io_worker([&io_context] { io_context.run(); });
    net::Router router{&database_executor};

    const auto send_request = [&](http::request<http::string_body> request) {
        std::promise<http::response<http::string_body>> completion;
        auto response_future = completion.get_future();
        boost::cobalt::spawn(
            io_context, router.HandleAsync(std::move(request)),
            [&completion](
                std::exception_ptr
                    exception,  // NOLINT (performance-unnecessary-value-param)
                http::response<http::string_body> response) {
                if (exception) {
                    completion.set_exception(exception);
                    return;
                }
                completion.set_value(std::move(response));
            });
        return response_future.get();
    };

    http::request<http::string_body> create_request{http::verb::post,
                                                    "/v1/products", 11};
    create_request.body() = R"({"name":"Milk","amount":1,"dimension":"l"})";
    const auto created_response = send_request(std::move(create_request));
    ASSERT_EQ(created_response.result(), http::status::created);
    const size_t id_start = created_response.body().find("\"id\":") + 5;
    const int64_t product_id =
        std::stoll(created_response.body().substr(id_start));
    const std::string product_target =
        "/v1/products/" + std::to_string(product_id);

    const auto fetched_response = send_request(
        http::request<http::string_body>{http::verb::get, product_target, 11});
    ASSERT_EQ(fetched_response.result(), http::status::ok);
    EXPECT_NE(fetched_response.body().find("Milk"), std::string::npos);

    http::request<http::string_body> update_request{http::verb::put,
                                                    product_target, 11};
    update_request.body() = R"({"name":"Milk","amount":2,"dimension":"l"})";
    const auto updated_response = send_request(std::move(update_request));
    ASSERT_EQ(updated_response.result(), http::status::ok);
    EXPECT_NE(updated_response.body().find("\"amount\":2"), std::string::npos);

    const auto deleted_response = send_request(http::request<http::string_body>{
        http::verb::delete_, product_target, 11});
    EXPECT_EQ(deleted_response.result(), http::status::no_content);

    const auto missing_response = send_request(
        http::request<http::string_body>{http::verb::get, product_target, 11});
    EXPECT_EQ(missing_response.result(), http::status::not_found);
    work_guard.reset();
}

TEST_F(TestHttpRouter, HandleAsync_RecipeCollections_ReturnNestedRecipes) {
    DatabaseExecutor database_executor{":memory:", 4};
    auto setup = database_executor.Submit([](RecipeService& service) {
        const auto product = service.CreateProduct(
            types::Product{"Water", 500, types::Dimension::Milliliter});
        if (!product.has_value()) {
            return std::expected<Recipe, Error>(
                std::unexpected(product.error()));
        }
        return service.CreateRecipe(Recipe{
            "Tea", {Product{"Water", 200, types::Dimension::Milliliter}}});
    });
    ASSERT_TRUE(setup.get().has_value());

    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);
    std::jthread io_worker([&io_context] { io_context.run(); });
    net::Router router{&database_executor};
    const auto send_request = [&](std::string_view target) {
        std::promise<http::response<http::string_body>> completion;
        auto response_future = completion.get_future();
        boost::cobalt::spawn(
            io_context,
            router.HandleAsync(
                http::request<http::string_body>{http::verb::get, target, 11}),
            [&completion](
                std::exception_ptr
                    exception,  // NOLINT (performance-unnecessary-value-param)
                http::response<http::string_body> response) {
                if (exception) {
                    completion.set_exception(exception);
                    return;
                }
                completion.set_value(std::move(response));
            });
        return response_future.get();
    };

    const auto recipes_response = send_request("/v1/recipes");
    const auto cookable_response = send_request("/v1/recipes/cookable");
    work_guard.reset();

    EXPECT_EQ(recipes_response.result(), http::status::ok);
    EXPECT_NE(recipes_response.body().find("Tea"), std::string::npos);
    EXPECT_NE(recipes_response.body().find("Water"), std::string::npos);
    EXPECT_EQ(cookable_response.result(), http::status::ok);
    EXPECT_NE(cookable_response.body().find("Tea"), std::string::npos);
}

TEST_F(TestHttpRouter, HandleAsync_RecipeCrudById_CompletesLifecycle) {
    DatabaseExecutor database_executor{":memory:", 4};
    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);
    std::jthread io_worker([&io_context] { io_context.run(); });
    net::Router router{&database_executor};

    const auto send_request = [&](http::request<http::string_body> request) {
        std::promise<http::response<http::string_body>> completion;
        auto response_future = completion.get_future();
        boost::cobalt::spawn(
            io_context, router.HandleAsync(std::move(request)),
            [&completion](
                std::exception_ptr
                    exception,  // NOLINT (performance-unnecessary-value-param)
                http::response<http::string_body> response) {
                if (exception) {
                    completion.set_exception(exception);
                    return;
                }
                completion.set_value(std::move(response));
            });
        return response_future.get();
    };

    http::request<http::string_body> create_request{http::verb::post,
                                                    "/v1/recipes", 11};
    create_request.body() =
        R"({"name":"Tea","ingredients":[{"name":"Water","amount":200,"dimension":"ml"}]})";
    const auto created_response = send_request(std::move(create_request));
    ASSERT_EQ(created_response.result(), http::status::created);
    const size_t id_start = created_response.body().find("\"id\":") + 5;
    const int64_t recipe_id =
        std::stoll(created_response.body().substr(id_start));
    const std::string recipe_target =
        "/v1/recipes/" + std::to_string(recipe_id);

    const auto fetched_response = send_request(
        http::request<http::string_body>{http::verb::get, recipe_target, 11});
    ASSERT_EQ(fetched_response.result(), http::status::ok);
    EXPECT_NE(fetched_response.body().find("Tea"), std::string::npos);
    EXPECT_NE(fetched_response.body().find("Water"), std::string::npos);

    http::request<http::string_body> update_request{http::verb::put,
                                                    recipe_target, 11};
    update_request.body() =
        R"({"name":"Iced tea","ingredients":[{"name":"Water","amount":300,"dimension":"ml"}]})";
    const auto updated_response = send_request(std::move(update_request));
    ASSERT_EQ(updated_response.result(), http::status::ok);
    EXPECT_NE(updated_response.body().find("Iced tea"), std::string::npos);
    EXPECT_NE(updated_response.body().find("\"amount\":300"),
              std::string::npos);

    const auto deleted_response = send_request(http::request<http::string_body>{
        http::verb::delete_, recipe_target, 11});
    EXPECT_EQ(deleted_response.result(), http::status::no_content);

    const auto missing_response = send_request(
        http::request<http::string_body>{http::verb::get, recipe_target, 11});
    EXPECT_EQ(missing_response.result(), http::status::not_found);
    work_guard.reset();
}

}  // namespace