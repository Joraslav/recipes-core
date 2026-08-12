#include "App.hpp"

#include "SQLiteCpp/Exception.h"

#include "DBManager.hpp"
#include "io/args/Args.hpp"
#include "io/IO.hpp"
#include "types/kitchen/Types.hpp"

#include <expected>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string_view>
#include <system_error>
#include <vector>

using db::DBManager;
using io::AppArgs;
using io::Args;
using io::HasAnyOutput;
using io::RecipeSelection;
using io::RunReports;
using types::Recipe;

namespace {

[[nodiscard]] DBManager CreateDbManager(const std::filesystem::path& db_path) {
    if (db_path.empty()) {
        return DBManager{};
    }
    return DBManager{std::string_view{db_path.native()}};
}

}  // namespace

namespace app {

std::expected<std::vector<Recipe>, std::error_code> Execute(
    const AppArgs& args) {
    try {
        DBManager db_manager = CreateDbManager(args.db_path);

        if (!args.products.empty()) {
            db_manager.InsertProducts(args.products);
        }
        if (!args.recipes.empty()) {
            db_manager.InsertRecipes(args.recipes);
        }

        if (args.recipe_selection == RecipeSelection::Cookable) {
            return db_manager.GetCookableRecipes();
        }
        return db_manager.GetAllRecipes();
    } catch (const SQLite::Exception& ex) {
        return std::unexpected(std::make_error_code(std::errc::io_error));
    }
}

std::expected<void, std::error_code> Run(const Args& args) {
    return Run(args, std::cout);
}

std::expected<void, std::error_code> Run(const Args& args, std::ostream& out) {
    if (args.ShowHelp()) {
        return {};
    }

    const AppArgs& app_args = args.App();
    const io::ArgsOut& out_args = args.Out();

    auto recipes_result = Execute(app_args);
    if (!recipes_result.has_value()) {
        return std::unexpected(recipes_result.error());
    }

    if (!HasAnyOutput(out_args)) {
        return {};
    }

    return RunReports(recipes_result.value(), out_args, out);
}

}  // namespace app
