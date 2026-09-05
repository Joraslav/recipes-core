#include "app/DatabaseExecutor.hpp"
#include "config/ServerConfig.hpp"
#include "io/args/Args.hpp"
#include "server/Server.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <iterator>
#include <mutex>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

using app::DatabaseExecutor;
using io::arg::Args;
using io::arg::BuildUsage;
using io::arg::ParseArgs;
using net::Server;

namespace {

struct StopState final {
    std::atomic_bool requested{false};
    std::condition_variable condition;
    std::mutex mutex;
};

StopState& GetStopState() noexcept {
    static StopState state;
    return state;
}

void RequestStop(int signal_number) noexcept {
    static_cast<void>(signal_number);
    auto& state = GetStopState();
    state.requested.store(true);
    state.condition.notify_one();
}

}  // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args_vec;
    args_vec.reserve(argc);
    std::ranges::transform(std::span(argv, argc), std::back_inserter(args_vec),
                           [](char* s) { return std::string_view(s); });

    const std::span<const std::string_view> in_args(args_vec);
    const std::string_view program_name =
        in_args.empty() ? "recipes" : in_args.front();

    auto args_result = ParseArgs(in_args);
    if (!args_result.has_value()) {
        std::cerr << args_result.error() << '\n';
        std::cerr << BuildUsage(program_name);
        return 2;
    }

    const Args& args = *args_result;
    if (args.ShowHelp()) {
        std::cout << BuildUsage(program_name);
        return 0;
    }

    if (args.App().server_config_path.empty()) {
        return 0;
    }

    const auto config_result =
        config::LoadServerConfig(args.App().server_config_path);
    if (!config_result.has_value()) {
        std::cerr << "Failed to load server configuration: "
                  << config_result.error() << '\n';
        return 3;
    }

    DatabaseExecutor database_executor{config_result->database_path,
                                       config_result->database_queue_capacity};
    Server server{database_executor, *config_result};
    const auto start_result = server.Start();
    if (!start_result.has_value()) {
        std::cerr << "Failed to start server: " << start_result.error() << '\n';
        return 4;
    }

    std::ignore = std::signal(SIGINT, RequestStop);
    std::ignore = std::signal(SIGTERM, RequestStop);
    std::cout << "HTTP port: " << server.HttpPort() << '\n';
    if (const auto https_port = server.HttpsPort(); https_port != 0) {
        std::cout << "HTTPS port: " << https_port << '\n';
    }
    auto& stop_state = GetStopState();
    std::unique_lock lock(stop_state.mutex);
    stop_state.condition.wait(
        lock, [&stop_state] { return stop_state.requested.load(); });
    server.Stop();

    return 0;
}
