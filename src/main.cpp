#include "app/App.hpp"
#include "io/args/Args.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args_vec;
    args_vec.reserve(argc);
    std::ranges::transform(std::span(argv, argc), std::back_inserter(args_vec),
                           [](char* s) { return std::string_view(s); });

    const std::span<const std::string_view> in_args(args_vec);
    const std::string_view program_name =
        in_args.empty() ? "recipes" : in_args.front();

    auto args_result = io::ParseArgs(in_args);
    if (!args_result.has_value()) {
        std::cerr << args_result.error() << '\n';
        std::cerr << io::BuildUsage(program_name);
        return 2;
    }

    const io::Args& args = *args_result;
    if (args.ShowHelp()) {
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
