#pragma once

#include "app/DatabaseExecutor.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/cobalt/task.hpp>

namespace net {

namespace http = boost::beast::http;

/**
 * @brief Routes HTTP requests to infrastructure and application handlers.
 * @details The database executor is non-owning and must outlive the router.
 */
class Router final {
 public:
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
    [[nodiscard]] static http::response<http::string_body> Handle(
        const http::request<http::string_body>& request);

    /**
     * @brief Asynchronously handles a request requiring application state.
     * @param request Request received from an HTTP or HTTPS session.
     * @return Coroutine yielding a ready-to-write HTTP response.
     */
    [[nodiscard]] boost::cobalt::task<http::response<http::string_body>>
    HandleAsync(http::request<http::string_body> request);

 private:
    app::DatabaseExecutor* database_executor_;
};

}  // namespace net
