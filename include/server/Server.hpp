#pragma once

#include "app/DatabaseExecutor.hpp"
#include "config/ServerConfig.hpp"
#include "server/Router.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/cobalt/task.hpp>

#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <vector>

namespace net {

namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace cobalt = boost::cobalt;

/**
 * @brief Runs the asynchronous plain HTTP listener.
 * @details The database executor is non-owning and must outlive the server.
 * The server owns network worker threads and stops them in its destructor.
 */
class Server final {
 public:
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

    /**
     * @brief Creates a server configured for the HTTP listener.
     * @param database_executor Executor used by router application handlers.
     * @param config Validated server configuration.
     * @pre `database_executor` outlives the server.
     */
    Server(app::DatabaseExecutor& database_executor,
           config::ServerConfig config);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    /**
     * @brief Binds the HTTP endpoint and starts accept/network workers.
     * @return Empty expected on success or a bind/listen error message.
     * @pre The HTTP listener is enabled in the configuration.
     */
    [[nodiscard]] std::expected<void, std::string> Start();

    /** @brief Stops accepting, closes the context and joins network workers. */
    void Stop() noexcept;

    /** @brief Returns the bound HTTP port, including an OS-assigned port. */
    [[nodiscard]] uint16_t HttpPort() const noexcept;

    /** @brief Returns the bound HTTPS port, or zero when HTTPS is disabled. */
    [[nodiscard]] uint16_t HttpsPort() const noexcept;

 private:
    using Tcp = asio::ip::tcp;

    [[nodiscard]] cobalt::task<void> AcceptLoop();
    [[nodiscard]] cobalt::task<void> Session(
        std::shared_ptr<Tcp::socket> socket);
    [[nodiscard]] cobalt::task<void> AcceptHttpsLoop();
    [[nodiscard]] cobalt::task<void> TlsSession(
        std::shared_ptr<Tcp::socket> socket);
    [[nodiscard]] std::expected<void, std::string> ConfigureTls();
    [[nodiscard]] static std::expected<void, std::string> ConfigureListener(
        Tcp::acceptor& acceptor, const config::ListenerConfig& config,
        std::string_view protocol);
    void RegisterSession(const std::shared_ptr<Tcp::socket>& socket);
    void UnregisterSession(const std::shared_ptr<Tcp::socket>& socket);
    void CloseSessions() noexcept;

    app::DatabaseExecutor* database_executor_;
    config::ServerConfig config_;
    asio::io_context io_context_;
    Tcp::acceptor acceptor_;
    Tcp::acceptor https_acceptor_;
    ssl::context tls_context_;
    Router router_;
    std::vector<std::jthread> workers_;
    std::mutex sessions_mutex_;
    std::unordered_set<std::shared_ptr<Tcp::socket>> sessions_;
    bool started_{false};
};

}  // namespace net
