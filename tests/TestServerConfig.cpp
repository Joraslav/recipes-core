#include "gtest/gtest.h"

#include "config/ServerConfig.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;
using config::LoadServerConfig;
using config::ServerConfig;
using config::ValidateServerConfig;

namespace {

struct ConfigFile final {
    std::string_view filename;
    std::string_view content;
};

class TestServerConfig : public ::testing::Test {
 protected:
    void SetUp() override {
        const auto timestamp = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        temp_dir_ =
            fs::temp_directory_path() / ("recipes_server_config_" + timestamp);
        fs::create_directories(temp_dir_);
    }

    void TearDown() override { fs::remove_all(temp_dir_); }

    [[nodiscard]] std::filesystem::path WriteConfig(
        const ConfigFile& config_file) const {
        const auto path = temp_dir_ / config_file.filename;
        std::ofstream output{path};
        output << config_file.content;
        return path;
    }

 private:
    fs::path temp_dir_;
};

TEST_F(TestServerConfig, LoadServerConfig_Json_ParsesValidatedConfiguration) {
    constexpr std::string_view kServerConfig = R"({
        "http": {
            "enabled": true,
            "address": "127.0.0.1",
            "port": 8080
        },
        "https": {
            "enabled": true,
            "address": "127.0.0.1",
            "port": 8443
        },
        "tls": {
            "certificate_path": "cert.pem",
            "private_key_path": "key.pem",
            "certificate_chain_path": "chain.pem"
        },
        "database_path": "recipes.db",
        "network_threads": 4,
        "database_queue_capacity": 32,
        "header_limit": 4096,
        "body_limit": 65536,
        "request_timeout_seconds": 10
    })";
    const auto config_path =
        WriteConfig({.filename = "server.json", .content = kServerConfig});

    const auto result = LoadServerConfig(config_path);

    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_TRUE(result->https.enabled);
    EXPECT_EQ(result->https.port, 8443);
    EXPECT_EQ(result->network_threads, 4U);
}

TEST_F(TestServerConfig, LoadServerConfig_Yaml_ParsesValidatedConfiguration) {
    const auto config_path =
        WriteConfig({.filename = "server.yaml",
                     .content = "http:\n"
                                "  enabled: true\n"
                                "  address: 127.0.0.1\n"
                                "  port: 8080\n"
                                "https:\n"
                                "  enabled: false\n"
                                "database_path: recipes.db\n"
                                "network_threads: 2\n"
                                "database_queue_capacity: 16\n"
                                "header_limit: 4096\n"
                                "body_limit: 65536\n"
                                "request_timeout_seconds: 10\n"});

    const auto result = LoadServerConfig(config_path);

    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_TRUE(result->http.enabled);
    EXPECT_FALSE(result->https.enabled);
    EXPECT_EQ(result->database_queue_capacity, 16U);
}

TEST_F(TestServerConfig, ValidateServerConfig_HttpsWithoutKey_ReturnsError) {
    ServerConfig config;
    config.https = {.enabled = true, .address = "127.0.0.1", .port = 8443};
    config.http.enabled = false;
    config.tls.certificate_path = "cert.pem";

    const auto result = ValidateServerConfig(config);

    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

}  // namespace