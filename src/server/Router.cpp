#include "Router.hpp"

#include "api/Json.hpp"
#include "api/Types.hpp"
#include "app/DatabaseExecutor.hpp"
#include "app/Error.hpp"
#include "app/RecipeService.hpp"
#include "types/kitchen/Types.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/cobalt/op.hpp>
#include <boost/cobalt/task.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using api::ErrorResponse;
using api::ParseProductRequest;
using api::ProductResponse;
using api::SerializeJson;
using app::DatabaseExecutor;
using app::Error;
using app::ErrorCode;
using app::RecipeService;
using types::Product;

namespace cobalt = boost::cobalt;

namespace {

[[nodiscard]] std::optional<types::Dimension> ParseDimension(
    std::string_view dimension) {
    if (dimension == "gr") {
        return types::Dimension::Gramm;
    }
    if (dimension == "kg") {
        return types::Dimension::Kilogramm;
    }
    if (dimension == "ml") {
        return types::Dimension::Milliliter;
    }
    if (dimension == "l") {
        return types::Dimension::Liter;
    }
    if (dimension == "pc") {
        return types::Dimension::Piece;
    }
    return std::nullopt;
}

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

[[nodiscard]] ProductResponse ToProductResponse(const Product& product) {
    return {.id = product.GetId().value_or(0),
            .name = product.GetName(),
            .amount = product.GetAmount(),
            .dimension = std::string(product.GetDimensionInString()),
            .manufacture = std::nullopt,
            .expiration = std::nullopt};
}

[[nodiscard]] net::http::response<net::http::string_body> MakeAppErrorResponse(
    const Error& error, unsigned version, bool keep_alive) {
    const auto status = error.GetCode() == ErrorCode::Overloaded
                            ? net::http::status::service_unavailable
                            : net::http::status::internal_server_error;
    const std::string code = error.GetCode() == ErrorCode::Overloaded
                                 ? "overloaded"
                                 : "storage_error";
    return MakeErrorResponse(status, version, keep_alive, code,
                             error.GetMessage());
}

}  // namespace

namespace net {

Router::Router(app::DatabaseExecutor* database_executor) noexcept
    : database_executor_(database_executor) {}

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

cobalt::task<http::response<http::string_body>> Router::HandleAsync(
    http::request<http::string_body> request) {
    if (request.target() != "/v1/products" ||
        (request.method() != http::verb::get &&
         request.method() != http::verb::post)) {
        co_return Handle(request);
    }
    if (database_executor_ == nullptr) {
        co_return MakeErrorResponse(http::status::service_unavailable,
                                    request.version(), request.keep_alive(),
                                    "service_unavailable",
                                    "Database service is unavailable");
    }

    if (request.method() == http::verb::post) {
        const auto product_request = ParseProductRequest(request.body());
        if (!product_request.has_value()) {
            co_return MakeErrorResponse(
                http::status::bad_request, request.version(),
                request.keep_alive(), "invalid_json", product_request.error());
        }
        const auto dimension = ParseDimension(product_request->dimension);
        if (!dimension.has_value()) {
            co_return MakeErrorResponse(http::status::bad_request,
                                        request.version(), request.keep_alive(),
                                        "validation_error",
                                        "Invalid product dimension");
        }
        const Product product{product_request->name, product_request->amount,
                              *dimension};
        const auto created_product = co_await database_executor_->AsyncSubmit(
            [product](RecipeService& service) {
                return service.CreateProduct(product);
            },
            cobalt::use_op);
        if (!created_product.has_value()) {
            co_return MakeAppErrorResponse(created_product.error(),
                                           request.version(),
                                           request.keep_alive());
        }
        const auto body = SerializeJson(ToProductResponse(*created_product));
        if (!body.has_value()) {
            co_return MakeErrorResponse(http::status::internal_server_error,
                                        request.version(), request.keep_alive(),
                                        "serialization_error",
                                        "Failed to serialize product");
        }
        co_return MakeJsonResponse(http::status::created, request.version(),
                                   request.keep_alive(), *body);
    }

    const auto products_result = co_await database_executor_->AsyncSubmit(
        [](RecipeService& service) { return service.GetProducts(); },
        cobalt::use_op);
    if (!products_result.has_value()) {
        co_return MakeAppErrorResponse(products_result.error(),
                                       request.version(), request.keep_alive());
    }

    std::vector<ProductResponse> response_products;
    response_products.reserve(products_result->size());
    for (const auto& product : *products_result) {
        response_products.push_back(ToProductResponse(product));
    }
    const auto body = SerializeJson(response_products);
    if (!body.has_value()) {
        co_return MakeErrorResponse(http::status::internal_server_error,
                                    request.version(), request.keep_alive(),
                                    "serialization_error",
                                    "Failed to serialize products");
    }
    co_return MakeJsonResponse(http::status::ok, request.version(),
                               request.keep_alive(), *body);
}

}  // namespace net