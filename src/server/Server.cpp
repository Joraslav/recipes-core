#include "Server.hpp"

#include "app/DatabaseExecutor.hpp"
#include "config/ServerConfig.hpp"

#include <boost/asio/detached.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/cobalt/op.hpp>
#include <boost/cobalt/spawn.hpp>
#include <boost/cobalt/task.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/error_code.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <string>
#include <utility>

namespace beast = boost::beast;
using app::DatabaseExecutor;
using config::ServerConfig;

namespace net {

Server::Server(DatabaseExecutor& database_executor, ServerConfig config)
    : database_executor_(&database_executor),
      config_(std::move(config)),
      acceptor_(io_context_),
      https_acceptor_(io_context_),
      tls_context_(ssl::context::tls_server),
      router_(database_executor_) {}

Server::~Server() { Stop(); }

std::expected<void, std::string> Server::Start() {
    if (started_) {
        return std::unexpected("Server is already started");
    }
    if (!config_.http.enabled) {
        return std::unexpected("HTTP listener is disabled");
    }

    boost::system::error_code error;
    const auto address = asio::ip::make_address(config_.http.address, error);
    if (error) {
        return std::unexpected(
            std::format("Invalid HTTP address: {}", error.message()));
    }

    error = acceptor_.open(address.is_v6() ? Tcp::v6() : Tcp::v4(), error);
    if (!error) {
        error = acceptor_.set_option(Tcp::acceptor::reuse_address(true), error);
    }
    if (!error) {
        error =
            acceptor_.bind(Tcp::endpoint{address, config_.http.port}, error);
    }
    if (!error) {
        error =
            acceptor_.listen(asio::socket_base::max_listen_connections, error);
    }
    if (error) {
        boost::system::error_code close_error;
        close_error = acceptor_.close(close_error);
        return std::unexpected(
            std::format("Failed to start HTTP listener: {}", error.message()));
    }

    if (config_.https.enabled) {
        const auto tls_result = ConfigureTls();
        if (!tls_result.has_value()) {
            boost::system::error_code close_error;
            close_error = acceptor_.close(close_error);
            return std::unexpected(tls_result.error());
        }
        const auto https_address =
            asio::ip::make_address(config_.https.address, error);
        if (error) {
            return std::unexpected(
                std::format("Invalid HTTPS address: {}", error.message()));
        }
        error = https_acceptor_.open(
            https_address.is_v6() ? Tcp::v6() : Tcp::v4(), error);
        if (!error) {
            error = https_acceptor_.set_option(
                Tcp::acceptor::reuse_address(true), error);
        }
        if (!error) {
            error = https_acceptor_.bind(
                Tcp::endpoint{https_address, config_.https.port}, error);
        }
        if (!error) {
            error = https_acceptor_.listen(
                asio::socket_base::max_listen_connections, error);
        }
        if (error) {
            boost::system::error_code close_error;
            close_error = https_acceptor_.close(close_error);
            close_error = acceptor_.close(close_error);
            return std::unexpected(std::format(
                "Failed to start HTTPS listener: {}", error.message()));
        }
    }

    started_ = true;
    cobalt::spawn(io_context_, AcceptLoop(), asio::detached);
    if (config_.https.enabled) {
        cobalt::spawn(io_context_, AcceptHttpsLoop(), asio::detached);
    }
    const size_t worker_count = std::max<size_t>(1, config_.network_threads);
    workers_.reserve(worker_count);
    for (size_t index = 0; index < worker_count; ++index) {
        workers_.emplace_back([this] { io_context_.run(); });
    }
    return {};
}

void Server::Stop() noexcept {
    if (!started_) {
        return;
    }
    started_ = false;
    boost::system::error_code error;
    error = acceptor_.close(error);
    error = https_acceptor_.close(error);
    io_context_.stop();
    workers_.clear();
}

uint16_t Server::HttpPort() const noexcept {
    if (!acceptor_.is_open()) {
        return 0;
    }
    boost::system::error_code error;
    const auto endpoint = acceptor_.local_endpoint(error);
    return error ? 0 : endpoint.port();
}

uint16_t Server::HttpsPort() const noexcept {
    if (!https_acceptor_.is_open()) {
        return 0;
    }
    boost::system::error_code error;
    const auto endpoint = https_acceptor_.local_endpoint(error);
    return error ? 0 : endpoint.port();
}

std::expected<void, std::string> Server::ConfigureTls() {
    if (config_.tls.certificate_path.empty() ||
        config_.tls.private_key_path.empty()) {
        return std::unexpected(
            "HTTPS requires certificate and private key paths");
    }
    try {
        tls_context_.set_options(
            ssl::context::default_workarounds | ssl::context::no_sslv2 |
            ssl::context::no_sslv3 | ssl::context::single_dh_use);
        tls_context_.use_certificate_chain_file(config_.tls.certificate_path);
        tls_context_.use_private_key_file(config_.tls.private_key_path,
                                          ssl::context::pem);
        if (!config_.tls.certificate_chain_path.empty()) {
            tls_context_.load_verify_file(config_.tls.certificate_chain_path);
        }
    } catch (const std::exception& exception) {
        return std::unexpected(
            std::format("Failed to configure TLS: {}", exception.what()));
    }
    return {};
}

cobalt::task<void> Server::AcceptLoop() {
    try {
        while (started_) {
            auto socket = co_await acceptor_.async_accept(cobalt::use_op);
            cobalt::spawn(io_context_, Session(std::move(socket)),
                          asio::detached);
        }
    } catch (const std::exception&) {
        co_return;
    }
}

cobalt::task<void> Server::Session(Tcp::socket socket) {
    try {
        beast::flat_buffer buffer;
        while (socket.is_open()) {
            Request request;
            co_await http::async_read(  // NOLINT (missing-includes)
                socket, buffer, request, cobalt::use_op);

            auto response = co_await router_.HandleAsync(std::move(request));
            const bool keep_alive = response.keep_alive();
            co_await http::async_write(  // NOLINT (missing-includes)
                socket, response, cobalt::use_op);
            if (!keep_alive) {
                boost::system::error_code error;
                error = socket.shutdown(Tcp::socket::shutdown_send, error);
                co_return;
            }
        }
    } catch (const std::exception&) {
        co_return;
    }
}

cobalt::task<void> Server::AcceptHttpsLoop() {
    try {
        while (started_) {
            auto socket = co_await https_acceptor_.async_accept(cobalt::use_op);
            cobalt::spawn(io_context_, TlsSession(std::move(socket)),
                          asio::detached);
        }
    } catch (const std::exception&) {
        co_return;
    }
}

cobalt::task<void> Server::TlsSession(Tcp::socket socket) {
    try {
        ssl::stream<Tcp::socket> stream{std::move(socket), tls_context_};
        co_await stream.async_handshake(ssl::stream_base::server,
                                        cobalt::use_op);

        beast::flat_buffer buffer;
        while (stream.next_layer().is_open()) {
            Request request;
            co_await http::async_read(  // NOLINT (missing-includes)
                stream, buffer, request, cobalt::use_op);
            auto response = co_await router_.HandleAsync(std::move(request));
            const bool keep_alive = response.keep_alive();
            co_await http::async_write(  // NOLINT (missing-includes)
                stream, response, cobalt::use_op);
            if (!keep_alive) {
                boost::system::error_code error;
                error = stream.shutdown(error);
                co_return;
            }
        }
    } catch (const std::exception&) {
        co_return;
    }
}

}  // namespace net
