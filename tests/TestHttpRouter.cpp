#include "gtest/gtest.h"

#include "app/DatabaseExecutor.hpp"
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

#include <exception>
#include <future>
#include <string>
#include <thread>
#include <utility>

using app::DatabaseExecutor;
using app::RecipeService;
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
            types::Product{"Egg", 4, types::Dimension::Piece});
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

}  // namespace