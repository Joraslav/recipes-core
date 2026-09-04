#pragma once

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/string_body_fwd.hpp>

namespace net {

namespace http = boost::beast::http;

/**
 * @brief Routes HTTP requests that do not require application state.
 * @details Resource CRUD routes will be added with asynchronous application
 * handlers. Returned responses use JSON bodies for a uniform API contract.
 */
class Router final {
 public:
    /**
     * @brief Handles a single parsed HTTP request.
     * @param request Request received from an HTTP or HTTPS session.
     * @return Ready-to-write HTTP response.
     * @exception_safety Strong guarantee.
     */
    [[nodiscard]] static http::response<http::string_body> Handle(
        const http::request<http::string_body>& request);
};

}  // namespace net
