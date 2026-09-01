#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace config {

/**
 * @brief Network binding settings for one HTTP or HTTPS listener.
 */
struct ListenerConfig final {
    bool enabled{false};
    std::string address{"127.0.0.1"};
    uint16_t port{};
};

/**
 * @brief Paths to TLS materials used by the HTTPS listener.
 */
struct TlsConfig final {
    std::string certificate_path;
    std::string private_key_path;
    std::string certificate_chain_path;
};

/**
 * @brief Fully parsed settings used to construct and run the server.
 * @details File paths are kept as strings for Glaze YAML compatibility and
 * converted to `std::filesystem::path` at the server startup boundary.
 */
struct ServerConfig final {
    ListenerConfig http{.enabled = true, .address = "127.0.0.1", .port = 8080};
    ListenerConfig https{};
    TlsConfig tls{};
    std::string database_path{"data/recipes.db"};
    size_t network_threads{1};
    size_t database_queue_capacity{128};
    size_t header_limit{8192};
    size_t body_limit{1048576};
    uint32_t request_timeout_seconds{30};
};

/**
 * @brief Loads and validates server settings from a JSON or YAML file.
 * @param config_path Path ending in `.json`, `.yaml` or `.yml`.
 * @return Validated configuration or a human-readable read, parse or
 * validation error.
 * @exception_safety Strong guarantee.
 */
[[nodiscard]] std::expected<ServerConfig, std::string> LoadServerConfig(
    const std::filesystem::path& config_path);
/**
 * @brief Validates listener, TLS and resource-limit settings.
 * @param config Configuration to validate.
 * @return Empty expected on success or a human-readable validation error.
 * @exception_safety Nothrow.
 */
[[nodiscard]] std::expected<void, std::string> ValidateServerConfig(
    const ServerConfig& config);

}  // namespace config