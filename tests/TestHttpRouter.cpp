#include "gtest/gtest.h"

#include "server/Router.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>

#include <string>

namespace {

namespace http = net::http;

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

}  // namespace