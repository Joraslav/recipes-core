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
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/impl/error.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/parser_fwd.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/cobalt/op.hpp>
#include <boost/cobalt/spawn.hpp>
#include <boost/cobalt/task.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace beast = boost::beast;
namespace chron = std::chrono;
using app::DatabaseExecutor;
using config::ServerConfig;

namespace net {

namespace {

template <typename Stream>
cobalt::task<void> SendPayloadLimitResponse(Stream stream, unsigned version) {
    http::response<http::string_body> response{http::status::payload_too_large,
                                               version};
    response.set(http::field::content_type, "application/json");
    response.keep_alive(false);
    response.body() =
        R"({"code":"payload_too_large","message":"Request exceeds configured limits"})";
    response.prepare_payload();
    try {
        co_await http::async_write(stream, response, cobalt::use_op);
    } catch (const std::exception&) {
        co_return;
    }
}

}  // namespace

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
    if (!config_.http.enabled && !config_.https.enabled) {
        return std::unexpected("At least one listener must be enabled");
    }

    if (config_.http.enabled) {
        const auto listener_result =
            ConfigureListener(acceptor_, config_.http, "HTTP");
        if (!listener_result.has_value()) {
            return std::unexpected(listener_result.error());
        }
    }

    if (config_.https.enabled) {
        const auto tls_result = ConfigureTls();
        if (!tls_result.has_value()) {
            boost::system::error_code close_error;
            close_error = acceptor_.close(close_error);
            return std::unexpected(tls_result.error());
        }
        const auto listener_result =
            ConfigureListener(https_acceptor_, config_.https, "HTTPS");
        if (!listener_result.has_value()) {
            boost::system::error_code close_error;
            close_error = https_acceptor_.close(close_error);
            close_error = acceptor_.close(close_error);
            return std::unexpected(listener_result.error());
        }
    }

    started_ = true;
    if (config_.http.enabled) {
        cobalt::spawn(io_context_, AcceptLoop(), asio::detached);
    }
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

std::expected<void, std::string> Server::ConfigureListener(
    Tcp::acceptor& acceptor, const config::ListenerConfig& config,
    std::string_view protocol) {
    boost::system::error_code error;
    const auto address = asio::ip::make_address(config.address, error);
    if (error) {
        return std::unexpected(
            std::format("Invalid {} address: {}", protocol, error.message()));
    }

    error = acceptor.open(address.is_v6() ? Tcp::v6() : Tcp::v4(), error);
    if (!error) {
        error = acceptor.set_option(Tcp::acceptor::reuse_address(true), error);
    }
    if (!error) {
        error = acceptor.bind(Tcp::endpoint{address, config.port}, error);
    }
    if (!error) {
        error =
            acceptor.listen(asio::socket_base::max_listen_connections, error);
    }
    if (error) {
        boost::system::error_code close_error;
        close_error = acceptor.close(close_error);
        return std::unexpected(std::format("Failed to start {} listener: {}",
                                           protocol, error.message()));
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
    CloseSessions();
    io_context_.stop();
    workers_.clear();
}

void Server::RegisterSession(const std::shared_ptr<Tcp::socket>& socket) {
    const std::scoped_lock lock(sessions_mutex_);
    sessions_.insert(socket);
}

void Server::UnregisterSession(const std::shared_ptr<Tcp::socket>& socket) {
    const std::scoped_lock lock(sessions_mutex_);
    sessions_.erase(socket);
}

void Server::CloseSessions() noexcept {
    const std::scoped_lock lock(sessions_mutex_);
    for (const auto& socket : sessions_) {
        boost::system::error_code error;
        error = socket->shutdown(Tcp::socket::shutdown_both, error);
        error = socket->close(error);
    }
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
            auto session_socket =
                std::make_shared<Tcp::socket>(std::move(socket));
            RegisterSession(session_socket);
            cobalt::spawn(io_context_, Session(std::move(session_socket)),
                          asio::detached);
        }
    } catch (const std::exception&) {
        co_return;
    }
}

cobalt::task<void> Server::Session(
    std::shared_ptr<Tcp::socket> session_socket) {
    try {
        beast::tcp_stream stream{std::move(*session_socket)};
        beast::flat_buffer buffer;
        while (stream.socket().is_open()) {
            stream.expires_after(
                chron::seconds{config_.request_timeout_seconds});
            http::request_parser<http::string_body> parser;
            parser.body_limit(config_.body_limit);
            parser.header_limit(config_.header_limit);
            bool payload_limit_exceeded = false;
            try {
                co_await http::async_read(  // NOLINT (missing-includes)
                    stream, buffer, parser, cobalt::use_op);
            } catch (const boost::system::system_error& exception) {
                payload_limit_exceeded =
                    exception.code() ==
                        http::make_error_code(http::error::body_limit) ||
                    exception.code() ==
                        http::make_error_code(http::error::header_limit);
            }
            if (payload_limit_exceeded) {
                co_await SendPayloadLimitResponse(std::move(stream), 11);
                co_return;
            }
            Request request = parser.release();

            auto response = co_await router_.HandleAsync(std::move(request));
            const bool keep_alive = response.keep_alive();
            stream.expires_after(
                chron::seconds{config_.request_timeout_seconds});
            co_await http::async_write(  // NOLINT (missing-includes)
                stream, response, cobalt::use_op);
            if (!keep_alive) {
                boost::system::error_code error;
                error =
                    stream.socket().shutdown(Tcp::socket::shutdown_send, error);
                UnregisterSession(session_socket);
                co_return;
            }
        }
    } catch (const std::exception&) {
        UnregisterSession(session_socket);
        co_return;
    }
    UnregisterSession(session_socket);
}

cobalt::task<void> Server::AcceptHttpsLoop() {
    try {
        while (started_) {
            auto socket = co_await https_acceptor_.async_accept(cobalt::use_op);
            auto session_socket =
                std::make_shared<Tcp::socket>(std::move(socket));
            RegisterSession(session_socket);
            cobalt::spawn(io_context_, TlsSession(std::move(session_socket)),
                          asio::detached);
        }
    } catch (const std::exception&) {
        co_return;
    }
}

cobalt::task<void> Server::TlsSession(
    std::shared_ptr<Tcp::socket> session_socket) {
    try {
        ssl::stream<beast::tcp_stream> stream{
            beast::tcp_stream{std::move(*session_socket)}, tls_context_};
        stream.next_layer().expires_after(
            chron::seconds{config_.request_timeout_seconds});
        co_await stream.async_handshake(ssl::stream_base::server,
                                        cobalt::use_op);

        beast::flat_buffer buffer;
        while (stream.next_layer().socket().is_open()) {
            stream.next_layer().expires_after(
                chron::seconds{config_.request_timeout_seconds});
            http::request_parser<http::string_body> parser;
            parser.body_limit(config_.body_limit);
            parser.header_limit(config_.header_limit);
            bool payload_limit_exceeded = false;
            try {
                co_await http::async_read(  // NOLINT (missing-includes)
                    stream, buffer, parser, cobalt::use_op);
            } catch (const boost::system::system_error& exception) {
                payload_limit_exceeded =
                    exception.code() ==
                        http::make_error_code(http::error::body_limit) ||
                    exception.code() ==
                        http::make_error_code(http::error::header_limit);
            }
            if (payload_limit_exceeded) {
                co_await SendPayloadLimitResponse(std::move(stream), 11);
                co_return;
            }
            Request request = parser.release();
            auto response = co_await router_.HandleAsync(std::move(request));
            const bool keep_alive = response.keep_alive();
            stream.next_layer().expires_after(
                chron::seconds{config_.request_timeout_seconds});
            co_await http::async_write(  // NOLINT (missing-includes)
                stream, response, cobalt::use_op);
            if (!keep_alive) {
                boost::system::error_code error;
                error = stream.next_layer().socket().shutdown(
                    Tcp::socket::shutdown_both, error);
                error = stream.next_layer().socket().close(error);
                UnregisterSession(session_socket);
                co_return;
            }
        }
    } catch (const std::exception&) {
        UnregisterSession(session_socket);
        co_return;
    }
    UnregisterSession(session_socket);
}

}  // namespace net
