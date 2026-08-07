#include "App.hpp"

#include "SQLiteCpp/Exception.h"

#include "DBManager.hpp"
#include "io/Args.hpp"
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

        if (args.recipe_selection == RecipeSelection::COOKABLE) {
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
    if (args.show_help) {
        return {};
    }

    auto recipes_result = Execute(args.app);
    if (!recipes_result.has_value()) {
        return std::unexpected(recipes_result.error());
    }

    if (!HasAnyOutput(args.out)) {
        return {};
    }

    return RunReports(recipes_result.value(), args.out, out);
}

}  // namespace app
