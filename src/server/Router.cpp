#include "Router.hpp"

#include "api/Json.hpp"
#include "api/Types.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>

#include <string>
#include <string_view>
#include <utility>

using api::ErrorResponse;
using api::SerializeJson;

namespace {

[[nodiscard]] net::http::response<net::http::string_body> MakeJsonResponse(
    net::http::status status, unsigned version, bool keep_alive,
    std::string body) {
    net::http::response<net::http::string_body> response{status, version};
    response.set(net::http::field::content_type, "application/json");
    response.keep_alive(keep_alive);
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

[[nodiscard]] net::http::response<net::http::string_body> MakeErrorResponse(
    net::http::status status, unsigned version, bool keep_alive,
    std::string_view code, std::string_view message) {
    const ErrorResponse error{.code = std::string{code},
                              .message = std::string{message}};
    const auto body = SerializeJson(error);
    return MakeJsonResponse(status, version, keep_alive,
                            body.value_or(R"({"code":"internal_error"})"));
}

}  // namespace

namespace net {

http::response<http::string_body> Router::Handle(
    const http::request<http::string_body>& request) {
    if (request.target() == "/healthz") {
        if (request.method() != http::verb::get) {
            return MakeErrorResponse(http::status::method_not_allowed,
                                     request.version(), request.keep_alive(),
                                     "method_not_allowed",
                                     "Only GET is supported for /healthz");
        }
        return MakeJsonResponse(http::status::ok, request.version(),
                                request.keep_alive(), R"({"status":"ok"})");
    }

    return MakeErrorResponse(http::status::not_found, request.version(),
                             request.keep_alive(), "not_found",
                             "The requested resource was not found");
}

}  // namespace net