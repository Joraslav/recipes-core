#pragma once

#include "app/DatabaseExecutor.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/cobalt/task.hpp>

#include <cstdint>
#include <optional>

namespace net {

namespace http = boost::beast::http;

/**
 * @brief Routes HTTP requests to infrastructure and application handlers.
 * @details The database executor is non-owning and must outlive the router.
 */
class Router final {
 public:
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

    /**
     * @brief Creates a router with an optional database executor.
     * @param database_executor Executor used by asynchronous resource routes.
     * @pre `database_executor`, when non-null, outlives this router.
     */
    explicit Router(
        app::DatabaseExecutor* database_executor = nullptr) noexcept;

    /**
     * @brief Handles a single parsed HTTP request.
     * @param request Request received from an HTTP or HTTPS session.
     * @return Ready-to-write HTTP response.
     * @exception_safety Strong guarantee.
     */
    [[nodiscard]] static Response Handle(const Request& request);

    /**
     * @brief Asynchronously handles a request requiring application state.
     * @param request Request received from an HTTP or HTTPS session.
     * @return Coroutine yielding a ready-to-write HTTP response.
     */
    [[nodiscard]] boost::cobalt::task<Response> HandleAsync(Request request);

 private:
    [[nodiscard]] boost::cobalt::task<Response> HandleProducts(
        Request request, std::optional<int64_t> product_id);
    [[nodiscard]] boost::cobalt::task<Response> CreateProduct(Request request);
    [[nodiscard]] boost::cobalt::task<Response> UpdateProduct(
        Request request, int64_t product_id);
    [[nodiscard]] boost::cobalt::task<Response> DeleteProduct(
        Request request, int64_t product_id);
    [[nodiscard]] boost::cobalt::task<Response> GetProduct(Request request,
                                                           int64_t product_id);
    [[nodiscard]] boost::cobalt::task<Response> ListProducts(Request request);
    [[nodiscard]] boost::cobalt::task<Response> HandleRecipes(Request request,
                                                              bool cookable);

    app::DatabaseExecutor* database_executor_;
};

}  // namespace net
