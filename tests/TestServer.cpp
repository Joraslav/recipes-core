#include "gtest/gtest.h"

#include "app/DatabaseExecutor.hpp"
#include "config/ServerConfig.hpp"
#include "server/Server.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/system/detail/error_code.hpp>

#include <array>
#include <string>

namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace http = beast::http;

using app::DatabaseExecutor;
using config::ServerConfig;
using net::Server;

#ifndef RECIPES_TEST_SOURCE_DIR
#define RECIPES_TEST_SOURCE_DIR "."
#endif

namespace {

class TestServer : public ::testing::Test {
 protected:
    DatabaseExecutor database_executor{":memory:", 8};
};

TEST_F(TestServer, Start_HealthRequest_ReturnsOkAndStops) {
    ServerConfig config;
    config.http = {.enabled = true, .address = "127.0.0.1", .port = 0};
    config.network_threads = 2;

    Server server{database_executor, config};
    const auto started = server.Start();
    ASSERT_TRUE(started.has_value()) << started.error();
    ASSERT_NE(server.HttpPort(), 0);

    asio::io_context client_context;
    asio::ip::tcp::socket socket{client_context};
    const auto address = asio::ip::make_address("127.0.0.1");
    socket.connect({address, server.HttpPort()});

    http::request<http::empty_body> request{http::verb::get, "/healthz", 11};
    request.keep_alive(false);
    http::write(socket, request);  // NOLINT (missing-includes)

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(socket, buffer, response);  // NOLINT (missing-includes)

    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_EQ(response.body(), R"({"status":"ok"})");
    server.Stop();
}

TEST_F(TestServer, Start_HttpsWithoutCredentials_ReturnsError) {
    ServerConfig config;
    config.http = {.enabled = true, .address = "127.0.0.1", .port = 0};
    config.https = {.enabled = true, .address = "127.0.0.1", .port = 0};

    Server server{database_executor, config};
    const auto started = server.Start();

    ASSERT_FALSE(started.has_value());
    EXPECT_NE(started.error().find("certificate"), std::string::npos);
    EXPECT_EQ(server.HttpPort(), 0);
    EXPECT_EQ(server.HttpsPort(), 0);
}

TEST_F(TestServer, Start_HttpsHealthRequest_ReturnsOkAndStops) {
    ServerConfig config;
    config.http.enabled = false;
    config.https = {.enabled = true, .address = "127.0.0.1", .port = 0};
    config.tls.certificate_path = std::string{RECIPES_TEST_SOURCE_DIR}.append(
        "/tests/data/certs/server.crt");
    config.tls.private_key_path = std::string{RECIPES_TEST_SOURCE_DIR}.append(
        "/tests/data/certs/server.key");
    config.network_threads = 2;

    Server server{database_executor, config};
    const auto started = server.Start();
    ASSERT_TRUE(started.has_value()) << started.error();
    ASSERT_NE(server.HttpsPort(), 0);

    asio::io_context client_context;
    ssl::context client_tls{ssl::context::tls_client};
    client_tls.set_verify_mode(ssl::verify_none);
    ssl::stream<asio::ip::tcp::socket> stream{client_context, client_tls};
    const auto address = asio::ip::make_address("127.0.0.1");
    stream.next_layer().connect({address, server.HttpsPort()});
    stream.handshake(ssl::stream_base::client);

    http::request<http::empty_body> request{http::verb::get, "/healthz", 11};
    request.keep_alive(false);
    http::write(stream, request);

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(stream, buffer, response);

    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_EQ(response.body(), R"({"status":"ok"})");
    server.Stop();
}

TEST_F(TestServer, Start_BodyLimitExceeded_ClosesConnection) {
    ServerConfig config;
    config.http = {.enabled = true, .address = "127.0.0.1", .port = 0};
    config.body_limit = 16;
    config.header_limit = 1024;

    Server server{database_executor, config};
    const auto started = server.Start();
    ASSERT_TRUE(started.has_value()) << started.error();

    asio::io_context client_context;
    asio::ip::tcp::socket socket{client_context};
    const auto address = asio::ip::make_address("127.0.0.1");
    socket.connect({address, server.HttpPort()});

    http::request<http::string_body> request{http::verb::post, "/v1/products",
                                             11};
    request.body() = std::string(64, 'x');
    request.prepare_payload();
    request.keep_alive(false);
    http::write(socket, request);

    boost::system::error_code error;
    std::array<char, 1> buffer{};
    socket.read_some(asio::buffer(buffer), error);

    EXPECT_TRUE(error == asio::error::eof ||
                error == asio::error::connection_reset);
    server.Stop();
}

TEST_F(TestServer, Stop_ActiveKeepAliveConnection_ClosesSocket) {
    ServerConfig config;
    config.http = {.enabled = true, .address = "127.0.0.1", .port = 0};

    Server server{database_executor, config};
    const auto started = server.Start();
    ASSERT_TRUE(started.has_value()) << started.error();

    asio::io_context client_context;
    asio::ip::tcp::socket socket{client_context};
    const auto address = asio::ip::make_address("127.0.0.1");
    socket.connect({address, server.HttpPort()});

    server.Stop();

    boost::system::error_code error;
    std::array<char, 1> buffer{};
    socket.read_some(asio::buffer(buffer), error);
    EXPECT_TRUE(error == asio::error::eof ||
                error == asio::error::connection_reset);
}

}  // namespace