#include "io/console/Report.hpp"
#include "io/json/Report.hpp"
#include "io/yaml/Report.hpp"
#include "types/kitchen/Types.hpp"

#include <filesystem>
#include <initializer_list>
#include <string_view>
#include <vector>

using namespace io;
using namespace types;
namespace fs = std::filesystem;

namespace {
Product MakeProduct(std::string_view name, int amount, Dimension dimension,
                    Dates dates = {}) {
    return Product{name, amount, dimension, dates};
}

Recipe MakeRecipe(std::string_view name,
                  std::initializer_list<Product> ingredients) {
    return Recipe{name, ingredients};
}
}  // namespace

int main() {
    const std::vector<Recipe> recipes{
        MakeRecipe("Pancake",
                   {
                       MakeProduct("Milk", 200, Dimension::MILLILITER),
                       MakeProduct("Flour", 150, Dimension::GRAMM),
                   }),
    };
    const fs::path json_out_path = "data/out.json";
    const fs::path yaml_out_path = "data/out.yaml";
    json::WriteRecipesJson(recipes, json_out_path);
    yaml::WriteRecipesYaml(recipes, yaml_out_path);
    cli::PrintRecipes(recipes, true);
    return 0;
}
