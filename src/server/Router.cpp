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

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace cobalt = boost::cobalt;
using namespace std::string_view_literals;

using api::ErrorResponse;
using api::ParseProductRequest;
using api::ParseRecipeRequest;
using api::ProductResponse;
using api::RecipeResponse;
using api::SerializeJson;
using app::DatabaseExecutor;
using app::Error;
using app::ErrorCode;
using app::RecipeService;
using types::Product;
using types::Recipe;

namespace {

[[nodiscard]] std::optional<int64_t> ParseProductId(std::string_view target) {
    constexpr std::string_view kPrefix = "/v1/products/";
    if (!target.starts_with(kPrefix) || target.size() == kPrefix.size()) {
        return std::nullopt;
    }
    const std::string_view id_text = target.substr(kPrefix.size());
    int64_t product_id{};
    const auto [end, error] =
        std::from_chars(id_text.begin(), id_text.end(), product_id);
    if (error != std::errc{} || end != id_text.end() || product_id <= 0) {
        return std::nullopt;
    }
    return product_id;
}

[[nodiscard]] std::optional<int64_t> ParseRecipeId(std::string_view target) {
    constexpr std::string_view kPrefix = "/v1/recipes/";
    if (!target.starts_with(kPrefix) || target.size() == kPrefix.size() ||
        target == "/v1/recipes/cookable") {
        return std::nullopt;
    }
    const std::string_view id_text = target.substr(kPrefix.size());
    int64_t recipe_id{};
    const auto [end, error] =
        std::from_chars(id_text.begin(), id_text.end(), recipe_id);
    if (error != std::errc{} || end != id_text.end() || recipe_id <= 0) {
        return std::nullopt;
    }
    return recipe_id;
}

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
    response.set(net::http::field::x_content_type_options, "nosniff");
    response.keep_alive(keep_alive);
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

struct AllowHeader final {
    std::string_view value;
};

struct ErrorPayload final {
    std::string_view code;
    std::string_view message;
};

[[nodiscard]] net::http::response<net::http::string_body> MakeErrorResponse(
    net::http::status status, unsigned version, bool keep_alive,
    ErrorPayload payload, std::optional<AllowHeader> allow = std::nullopt) {
    const ErrorResponse error{.code = std::string{payload.code},
                              .message = std::string{payload.message}};
    const auto body = SerializeJson(error);
    auto response =
        MakeJsonResponse(status, version, keep_alive,
                         body.value_or(R"({"code":"internal_error"})"));
    if (allow.has_value()) {
        response.set(net::http::field::allow, allow->value);
    }
    return response;
}

[[nodiscard]] ProductResponse ToProductResponse(const Product& product) {
    return {.id = product.GetId().value_or(0),
            .name = product.GetName(),
            .amount = product.GetAmount(),
            .dimension = std::string(product.GetDimensionInString()),
            .manufacture = std::nullopt,
            .expiration = std::nullopt};
}

[[nodiscard]] RecipeResponse ToRecipeResponse(const Recipe& recipe) {
    RecipeResponse response{.id = recipe.GetId().value_or(0),
                            .name = recipe.GetName(),
                            .ingredients = {}};
    response.ingredients.reserve(recipe.GetIngredients().size());
    for (const auto& ingredient : recipe.GetIngredients()) {
        response.ingredients.push_back(ToProductResponse(ingredient));
    }
    return response;
}

[[nodiscard]] net::http::response<net::http::string_body> MakeAppErrorResponse(
    const Error& error, unsigned version, bool keep_alive) {
    net::http::status status = net::http::status::internal_server_error;
    const char* code = "storage_error";
    switch (error.GetCode()) {
        case ErrorCode::Validation:
            status = net::http::status::unprocessable_entity;
            code = "validation_error";
            break;
        case ErrorCode::NotFound:
            status = net::http::status::not_found;
            code = "not_found";
            break;
        case ErrorCode::Conflict:
            status = net::http::status::conflict;
            code = "conflict";
            break;
        case ErrorCode::Overloaded:
            status = net::http::status::service_unavailable;
            code = "overloaded";
            break;
        case ErrorCode::Storage:
            code = "internal_error";
            break;
    }
    std::string_view message = "Storage operation failed";
    if (error.GetCode() != ErrorCode::Storage) {
        message = error.GetMessage();
    }
    return MakeErrorResponse(status, version, keep_alive,
                             {.code = code, .message = message});
}

[[nodiscard]] bool IsJsonContentType(
    const net::http::request<net::http::string_body>& request) {
    const auto content_type = request[net::http::field::content_type];
    return content_type.starts_with("application/json");
}

[[nodiscard]] net::http::response<net::http::string_body>
MakeMethodNotAllowedResponse(const net::Router::Request& request,
                             AllowHeader allow) {
    return MakeErrorResponse(
        net::http::status::method_not_allowed, request.version(),
        request.keep_alive(),
        {.code = "method_not_allowed", .message = "HTTP method is not allowed"},
        allow);
}

[[nodiscard]] std::optional<Product> ParseProduct(
    std::string_view body, net::http::status& error_status,
    std::string& error_message) {
    const auto product_request = ParseProductRequest(body);
    if (!product_request.has_value()) {
        error_status = net::http::status::bad_request;
        error_message = product_request.error();
        return std::nullopt;
    }
    const auto dimension = ParseDimension(product_request->dimension);
    if (!dimension.has_value()) {
        error_status = net::http::status::unprocessable_entity;
        error_message = "Invalid product dimension";
        return std::nullopt;
    }
    return Product{product_request->name, product_request->amount, *dimension};
}

[[nodiscard]] std::optional<Recipe> ParseRecipe(std::string_view body,
                                                net::http::status& error_status,
                                                std::string& error_message) {
    const auto recipe_request = ParseRecipeRequest(body);
    if (!recipe_request.has_value()) {
        error_status = net::http::status::bad_request;
        error_message = recipe_request.error();
        return std::nullopt;
    }

    std::vector<Product> ingredients;
    ingredients.reserve(recipe_request->ingredients.size());
    for (const auto& ingredient_request : recipe_request->ingredients) {
        const auto dimension = ParseDimension(ingredient_request.dimension);
        if (!dimension.has_value()) {
            error_status = net::http::status::unprocessable_entity;
            error_message = "Invalid recipe ingredient dimension";
            return std::nullopt;
        }
        ingredients.emplace_back(ingredient_request.name,
                                 ingredient_request.amount, *dimension);
    }
    return Recipe{recipe_request->name, std::move(ingredients)};
}

}  // namespace

namespace net {

Router::Router(DatabaseExecutor* database_executor) noexcept
    : database_executor_(database_executor) {}

http::response<http::string_body> Router::Handle(
    const http::request<http::string_body>& request) {
    if (request.target() == "/healthz") {
        if (request.method() != http::verb::get) {
            return MakeErrorResponse(
                http::status::method_not_allowed, request.version(),
                request.keep_alive(),
                {.code = "method_not_allowed",
                 .message = "Only GET is supported for /healthz"});
        }
        return MakeJsonResponse(http::status::ok, request.version(),
                                request.keep_alive(), R"({"status":"ok"})");
    }

    if (request.target() == "/v1/products" &&
        request.method() != http::verb::get &&
        request.method() != http::verb::post) {
        return MakeMethodNotAllowedResponse(request,
                                            AllowHeader{"GET, POST"sv});
    }
    if (request.target() == "/v1/recipes" &&
        request.method() != http::verb::get &&
        request.method() != http::verb::post) {
        return MakeMethodNotAllowedResponse(request,
                                            AllowHeader{"GET, POST"sv});
    }
    if (request.target() == "/v1/recipes/cookable" &&
        request.method() != http::verb::get) {
        return MakeMethodNotAllowedResponse(request, AllowHeader{"GET"sv});
    }
    if (ParseProductId(request.target()).has_value() &&
        request.method() != http::verb::get &&
        request.method() != http::verb::put &&
        request.method() != http::verb::delete_) {
        return MakeMethodNotAllowedResponse(request,
                                            AllowHeader{"GET, PUT, DELETE"sv});
    }
    if (ParseRecipeId(request.target()).has_value() &&
        request.method() != http::verb::get &&
        request.method() != http::verb::put &&
        request.method() != http::verb::delete_) {
        return MakeMethodNotAllowedResponse(request,
                                            AllowHeader{"GET, PUT, DELETE"sv});
    }

    return MakeErrorResponse(
        http::status::not_found, request.version(), request.keep_alive(),
        {.code = "not_found",
         .message = "The requested resource was not found"});
}

cobalt::task<Router::Response> Router::HandleProducts(
    Request request, std::optional<int64_t> product_id) {
    if (database_executor_ == nullptr) {
        co_return MakeErrorResponse(
            http::status::service_unavailable, request.version(),
            request.keep_alive(),
            {.code = "service_unavailable",
             .message = "Database service is unavailable"});
    }

    switch (request.method()) {
        case http::verb::post:
            co_return co_await CreateProduct(std::move(request));
        case http::verb::put:
            co_return co_await UpdateProduct(std::move(request),
                                             product_id.value());
        case http::verb::delete_:
            co_return co_await DeleteProduct(std::move(request),
                                             product_id.value());
        case http::verb::get:
            if (product_id.has_value()) {
                co_return co_await GetProduct(std::move(request),
                                              product_id.value());
            }
            co_return co_await ListProducts(std::move(request));
        default:
            co_return Handle(request);
    }
}

cobalt::task<Router::Response> Router::CreateProduct(Request request) {
    if (!IsJsonContentType(request)) {
        co_return MakeErrorResponse(
            http::status::unsupported_media_type, request.version(),
            request.keep_alive(),
            {.code = "unsupported_media_type",
             .message = "Content-Type must be application/json"});
    }
    http::status parse_error_status = http::status::bad_request;
    std::string parse_error_message;
    const auto product =
        ParseProduct(request.body(), parse_error_status, parse_error_message);
    if (!product.has_value()) {
        co_return MakeErrorResponse(
            parse_error_status, request.version(), request.keep_alive(),
            {.code = "invalid_json", .message = parse_error_message});
    }
    const auto created_product = co_await database_executor_->AsyncSubmit(
        [product = *product](RecipeService& service) {
            return service.CreateProduct(product);
        },
        cobalt::use_op);
    if (!created_product.has_value()) {
        co_return MakeAppErrorResponse(created_product.error(),
                                       request.version(), request.keep_alive());
    }
    const auto body = SerializeJson(ToProductResponse(*created_product));
    if (!body.has_value()) {
        co_return MakeErrorResponse(http::status::internal_server_error,
                                    request.version(), request.keep_alive(),
                                    {.code = "serialization_error",
                                     .message = "Failed to serialize product"});
    }
    co_return MakeJsonResponse(http::status::created, request.version(),
                               request.keep_alive(), *body);
}

cobalt::task<Router::Response> Router::UpdateProduct(Request request,
                                                     int64_t product_id) {
    if (!IsJsonContentType(request)) {
        co_return MakeErrorResponse(
            http::status::unsupported_media_type, request.version(),
            request.keep_alive(),
            {.code = "unsupported_media_type",
             .message = "Content-Type must be application/json"});
    }
    http::status parse_error_status = http::status::bad_request;
    std::string parse_error_message;
    const auto product =
        ParseProduct(request.body(), parse_error_status, parse_error_message);
    if (!product.has_value()) {
        co_return MakeErrorResponse(
            parse_error_status, request.version(), request.keep_alive(),
            {.code = "invalid_json", .message = parse_error_message});
    }
    const auto updated = co_await database_executor_->AsyncSubmit(
        [product_id, product = *product](RecipeService& service) {
            return service.UpdateProduct(product_id, product);
        },
        cobalt::use_op);
    if (!updated.has_value()) {
        co_return MakeAppErrorResponse(updated.error(), request.version(),
                                       request.keep_alive());
    }
    co_return co_await GetProduct(std::move(request), product_id);
}

cobalt::task<Router::Response> Router::DeleteProduct(Request request,
                                                     int64_t product_id) {
    const auto deleted = co_await database_executor_->AsyncSubmit(
        [product_id](RecipeService& service) {
            return service.DeleteProduct(product_id);
        },
        cobalt::use_op);
    if (!deleted.has_value()) {
        co_return MakeAppErrorResponse(deleted.error(), request.version(),
                                       request.keep_alive());
    }
    http::response<http::string_body> response{http::status::no_content,
                                               request.version()};
    response.keep_alive(request.keep_alive());
    co_return response;
}

cobalt::task<Router::Response> Router::GetProduct(Request request,
                                                  int64_t product_id) {
    const auto product_result = co_await database_executor_->AsyncSubmit(
        [product_id](RecipeService& service) {
            return service.GetProduct(product_id);
        },
        cobalt::use_op);
    if (!product_result.has_value()) {
        co_return MakeAppErrorResponse(product_result.error(),
                                       request.version(), request.keep_alive());
    }
    if (!product_result->has_value()) {
        co_return MakeErrorResponse(
            http::status::not_found, request.version(), request.keep_alive(),
            {.code = "not_found", .message = "Product was not found"});
    }
    const auto body = SerializeJson(ToProductResponse(product_result->value()));
    if (!body.has_value()) {
        co_return MakeErrorResponse(http::status::internal_server_error,
                                    request.version(), request.keep_alive(),
                                    {.code = "serialization_error",
                                     .message = "Failed to serialize product"});
    }
    co_return MakeJsonResponse(http::status::ok, request.version(),
                               request.keep_alive(), *body);
}

cobalt::task<Router::Response> Router::ListProducts(Request request) {
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
        co_return MakeErrorResponse(
            http::status::internal_server_error, request.version(),
            request.keep_alive(),
            {.code = "serialization_error",
             .message = "Failed to serialize products"});
    }
    co_return MakeJsonResponse(http::status::ok, request.version(),
                               request.keep_alive(), *body);
}

cobalt::task<Router::Response> Router::HandleRecipes(Request request,
                                                     bool cookable) {
    if (database_executor_ == nullptr) {
        co_return MakeErrorResponse(
            http::status::service_unavailable, request.version(),
            request.keep_alive(),
            {.code = "service_unavailable",
             .message = "Database service is unavailable"});
    }
    const auto recipes_result = co_await database_executor_->AsyncSubmit(
        [cookable](RecipeService& service) {
            return cookable ? service.GetCookableRecipes()
                            : service.GetRecipes();
        },
        cobalt::use_op);
    if (!recipes_result.has_value()) {
        co_return MakeAppErrorResponse(recipes_result.error(),
                                       request.version(), request.keep_alive());
    }
    std::vector<RecipeResponse> response_recipes;
    response_recipes.reserve(recipes_result->size());
    for (const auto& recipe : *recipes_result) {
        response_recipes.push_back(ToRecipeResponse(recipe));
    }
    const auto body = SerializeJson(response_recipes);
    if (!body.has_value()) {
        co_return MakeErrorResponse(http::status::internal_server_error,
                                    request.version(), request.keep_alive(),
                                    {.code = "serialization_error",
                                     .message = "Failed to serialize recipes"});
    }
    co_return MakeJsonResponse(http::status::ok, request.version(),
                               request.keep_alive(), *body);
}

cobalt::task<Router::Response> Router::CreateRecipe(Request request) {
    if (!IsJsonContentType(request)) {
        co_return MakeErrorResponse(
            http::status::unsupported_media_type, request.version(),
            request.keep_alive(),
            {.code = "unsupported_media_type",
             .message = "Content-Type must be application/json"});
    }
    http::status parse_error_status = http::status::bad_request;
    std::string parse_error_message;
    const auto recipe =
        ParseRecipe(request.body(), parse_error_status, parse_error_message);
    if (!recipe.has_value()) {
        co_return MakeErrorResponse(
            parse_error_status, request.version(), request.keep_alive(),
            {.code = "invalid_json", .message = parse_error_message});
    }

    const auto created_recipe = co_await database_executor_->AsyncSubmit(
        [recipe = *recipe](RecipeService& service) {
            return service.CreateRecipe(recipe);
        },
        cobalt::use_op);
    if (!created_recipe.has_value()) {
        co_return MakeAppErrorResponse(created_recipe.error(),
                                       request.version(), request.keep_alive());
    }
    const auto body = SerializeJson(ToRecipeResponse(*created_recipe));
    if (!body.has_value()) {
        co_return MakeErrorResponse(http::status::internal_server_error,
                                    request.version(), request.keep_alive(),
                                    {.code = "serialization_error",
                                     .message = "Failed to serialize recipe"});
    }
    co_return MakeJsonResponse(http::status::created, request.version(),
                               request.keep_alive(), *body);
}

cobalt::task<Router::Response> Router::UpdateRecipe(Request request,
                                                    int64_t recipe_id) {
    if (!IsJsonContentType(request)) {
        co_return MakeErrorResponse(
            http::status::unsupported_media_type, request.version(),
            request.keep_alive(),
            {.code = "unsupported_media_type",
             .message = "Content-Type must be application/json"});
    }
    http::status parse_error_status = http::status::bad_request;
    std::string parse_error_message;
    const auto recipe =
        ParseRecipe(request.body(), parse_error_status, parse_error_message);
    if (!recipe.has_value()) {
        co_return MakeErrorResponse(
            parse_error_status, request.version(), request.keep_alive(),
            {.code = "invalid_json", .message = parse_error_message});
    }

    const auto updated = co_await database_executor_->AsyncSubmit(
        [recipe_id, recipe = *recipe](RecipeService& service) {
            return service.UpdateRecipe(recipe_id, recipe);
        },
        cobalt::use_op);
    if (!updated.has_value()) {
        co_return MakeAppErrorResponse(updated.error(), request.version(),
                                       request.keep_alive());
    }
    co_return co_await GetRecipe(std::move(request), recipe_id);
}

cobalt::task<Router::Response> Router::DeleteRecipe(Request request,
                                                    int64_t recipe_id) {
    const auto deleted = co_await database_executor_->AsyncSubmit(
        [recipe_id](RecipeService& service) {
            return service.DeleteRecipe(recipe_id);
        },
        cobalt::use_op);
    if (!deleted.has_value()) {
        co_return MakeAppErrorResponse(deleted.error(), request.version(),
                                       request.keep_alive());
    }
    http::response<http::string_body> response{http::status::no_content,
                                               request.version()};
    response.keep_alive(request.keep_alive());
    co_return response;
}

cobalt::task<Router::Response> Router::GetRecipe(Request request,
                                                 int64_t recipe_id) {
    const auto recipe_result = co_await database_executor_->AsyncSubmit(
        [recipe_id](RecipeService& service) {
            return service.GetRecipe(recipe_id);
        },
        cobalt::use_op);
    if (!recipe_result.has_value()) {
        co_return MakeAppErrorResponse(recipe_result.error(), request.version(),
                                       request.keep_alive());
    }
    if (!recipe_result->has_value()) {
        co_return MakeErrorResponse(
            http::status::not_found, request.version(), request.keep_alive(),
            {.code = "not_found", .message = "Recipe was not found"});
    }
    const auto body = SerializeJson(ToRecipeResponse(recipe_result->value()));
    if (!body.has_value()) {
        co_return MakeErrorResponse(http::status::internal_server_error,
                                    request.version(), request.keep_alive(),
                                    {.code = "serialization_error",
                                     .message = "Failed to serialize recipe"});
    }
    co_return MakeJsonResponse(http::status::ok, request.version(),
                               request.keep_alive(), *body);
}

cobalt::task<Router::Response> Router::HandleAsync(Request request) {
    const bool is_product_collection = request.target() == "/v1/products";
    const auto product_id = ParseProductId(request.target());
    const bool is_product_item = product_id.has_value();
    const bool is_recipe_collection = request.target() == "/v1/recipes";
    const bool is_cookable_recipes = request.target() == "/v1/recipes/cookable";
    const auto recipe_id = ParseRecipeId(request.target());
    const bool is_recipe_item = recipe_id.has_value();

    if (is_product_collection || is_product_item) {
        const bool collection_method =
            is_product_collection && (request.method() == http::verb::get ||
                                      request.method() == http::verb::post);
        const bool item_method =
            is_product_item && (request.method() == http::verb::get ||
                                request.method() == http::verb::put ||
                                request.method() == http::verb::delete_);
        if (collection_method || item_method) {
            co_return co_await HandleProducts(std::move(request), product_id);
        }
    }
    if (is_recipe_collection || is_cookable_recipes) {
        if (database_executor_ == nullptr) {
            co_return MakeErrorResponse(
                http::status::service_unavailable, request.version(),
                request.keep_alive(),
                {.code = "service_unavailable",
                 .message = "Database service is unavailable"});
        }
        if (is_recipe_collection && request.method() == http::verb::post) {
            co_return co_await CreateRecipe(std::move(request));
        }
        if (request.method() == http::verb::get) {
            co_return co_await HandleRecipes(std::move(request),
                                             is_cookable_recipes);
        }
    }
    if (is_recipe_item) {
        if (database_executor_ == nullptr) {
            co_return MakeErrorResponse(
                http::status::service_unavailable, request.version(),
                request.keep_alive(),
                {.code = "service_unavailable",
                 .message = "Database service is unavailable"});
        }
        switch (request.method()) {
            case http::verb::get:
                co_return co_await GetRecipe(std::move(request),
                                             recipe_id.value());
            case http::verb::put:
                co_return co_await UpdateRecipe(std::move(request),
                                                recipe_id.value());
            case http::verb::delete_:
                co_return co_await DeleteRecipe(std::move(request),
                                                recipe_id.value());
            default:
                break;
        }
    }
    co_return Handle(request);
}

}  // namespace net