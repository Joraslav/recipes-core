#include "app/App.hpp"
#include "io/Args.hpp"

#include <cstddef>
#include <iostream>
#include <span>
#include <string_view>

int main(int argc, const char* const* argv) {
    const auto argv_span =
        std::span<const char* const>(argv, static_cast<size_t>(argc));
    const std::string_view program_name =
        argv_span.empty() ? "recipes" : argv_span.front();

    auto args_result = io::ParseArgs(argv_span);
    if (!args_result.has_value()) {
        std::cerr << args_result.error() << '\n';
        std::cerr << io::BuildUsage(program_name);
        return 2;
    }

    const io::Args& args = *args_result;
    if (args.show_help) {
        std::cout << io::BuildUsage(program_name);
        return 0;
    }

    auto run_result = app::Run(args);
    if (!run_result.has_value()) {
        std::cerr << run_result.error() << '\n';
        return 1;
    }

    return 0;
}
