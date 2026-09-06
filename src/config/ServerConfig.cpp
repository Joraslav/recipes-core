#include "ServerConfig.hpp"

#include <glaze/core/common.hpp>
#include <glaze/forward.hpp>
#include <glaze/json/read.hpp>
#include <glaze/yaml/read.hpp>

#include <algorithm>
#include <cctype>
#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace fs = std::filesystem;
using config::ListenerConfig;
using config::ServerConfig;
using config::TlsConfig;

namespace glz {

template <>
struct meta<ListenerConfig> {
    using T = ListenerConfig;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value = object(
        "enabled", &T::enabled, "address", &T::address, "port", &T::port);
};

template <>
struct meta<TlsConfig> {
    using T = TlsConfig;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value =
        object("certificate_path", &T::certificate_path, "private_key_path",
               &T::private_key_path, "certificate_chain_path",
               &T::certificate_chain_path, "server_name", &T::server_name,
               "enable_hsts", &T::enable_hsts);
};

template <>
struct meta<ServerConfig> {
    using T = ServerConfig;
    // Glaze meta contract requires the member name `value`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[maybe_unused]] static constexpr auto value = object(
        "http", &T::http, "https", &T::https, "tls", &T::tls, "database_path",
        &T::database_path, "network_threads", &T::network_threads,
        "database_queue_capacity", &T::database_queue_capacity, "header_limit",
        &T::header_limit, "body_limit", &T::body_limit,
        "request_timeout_seconds", &T::request_timeout_seconds,
        "shutdown_timeout_seconds", &T::shutdown_timeout_seconds);
};

}  // namespace glz

namespace {

[[nodiscard]] std::expected<std::string, std::string> ReadConfigFile(
    const fs::path& config_path) {
    std::ifstream input{config_path, std::ios::binary};
    if (!input.is_open()) {
        return std::unexpected("Failed to open configuration file: " +
                               config_path.string());
    }
    return std::string{std::istreambuf_iterator<char>{input}, {}};
}

[[nodiscard]] std::string LowercaseExtension(const fs::path& config_path) {
    std::string extension = config_path.extension().string();
    std::ranges::transform(
        extension, extension.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return extension;
}

template <typename ReadFunction>
std::expected<ServerConfig, std::string> ParseConfig(std::string_view text,
                                                     ReadFunction&& read) {
    ServerConfig config;
    if (const auto error = std::forward<ReadFunction>(read)(config, text);
        error) {
        return std::unexpected(glz::format_error(error, text));
    }
    return config;
}

}  // namespace

namespace config {

std::expected<ServerConfig, std::string> LoadServerConfig(
    const fs::path& config_path) {
    const auto file_contents = ReadConfigFile(config_path);
    if (!file_contents) {
        return std::unexpected(file_contents.error());
    }

    const std::string extension = LowercaseExtension(config_path);
    std::expected<ServerConfig, std::string> config = std::unexpected(
        "Unsupported configuration file extension: " + extension);
    if (extension == ".json") {
        config = ParseConfig(*file_contents,
                             [](ServerConfig& value, std::string_view text) {
                                 return glz::read_json(value, text);
                             });
    } else if (extension == ".yaml" || extension == ".yml") {
        config = ParseConfig(*file_contents,
                             [](ServerConfig& value, std::string_view text) {
                                 return glz::read_yaml(value, text);
                             });
    }
    if (!config) {
        return std::unexpected(config.error());
    }
    if (const auto validation = ValidateServerConfig(*config); !validation) {
        return std::unexpected(validation.error());
    }
    return config;
}

std::expected<void, std::string> ValidateServerConfig(
    const ServerConfig& config) {
    if (!config.http.enabled && !config.https.enabled) {
        return std::unexpected("At least one listener must be enabled");
    }
    if (config.network_threads == 0) {
        return std::unexpected("Network thread count must be positive");
    }
    if (config.database_queue_capacity == 0) {
        return std::unexpected("Database queue capacity must be positive");
    }
    if (config.header_limit == 0 || config.body_limit == 0 ||
        config.request_timeout_seconds == 0) {
        return std::unexpected("Request limits and timeout must be positive");
    }
    if (config.shutdown_timeout_seconds == 0) {
        return std::unexpected("Shutdown timeout must be positive");
    }
    if (config.https.enabled && (config.tls.certificate_path.empty() ||
                                 config.tls.private_key_path.empty())) {
        return std::unexpected(
            "HTTPS requires certificate and private key paths");
    }
    if (config.http.enabled && config.https.enabled &&
        config.http.address == config.https.address &&
        config.http.port == config.https.port) {
        return std::unexpected(
            "HTTP and HTTPS listeners must use distinct endpoints");
    }
    return {};
}

}  // namespace config