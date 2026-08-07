#include "DBManager.hpp"
#include "types/kitchen/Types.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <random>
#include <string_view>
#include <vector>

using namespace db;
using namespace types;

namespace {

constexpr std::string_view kDbPath = "data/table.db";

struct DateRange final {
    int start_year{2020};
    int end_year{2026};
};

[[nodiscard]] Dates MakeRandomDate(DateRange range = {}) noexcept {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    const auto start = std::chrono::sys_days{std::chrono::year_month_day{
        std::chrono::year{range.start_year}, std::chrono::month{1},
        std::chrono::day{1}}};
    const auto end = std::chrono::sys_days{std::chrono::year_month_day{
        std::chrono::year{range.end_year}, std::chrono::month{12},
        std::chrono::day{31}}};

    const auto range_days =
        std::chrono::duration_cast<std::chrono::days>(end - start).count();
    std::uniform_int_distribution<long long> dist(0, range_days);
    const auto manufacture = start + std::chrono::days{dist(gen)};

    std::uniform_int_distribution<int> expiration_dist(1, 365);
    const auto expiration =
        manufacture + std::chrono::days{expiration_dist(gen)};

    return Dates{.manufacture = manufacture, .expiration = expiration};
}

[[nodiscard]] std::vector<Product> MakeProducts() {
    static constexpr std::array<std::string_view, 12> kProductNames = {
        "Milk", "Bread", "Eggs",   "Butter", "Cheese", "Chicken",
        "Rice", "Pasta", "Tomato", "Apple",  "Banana", "Potato"};
    static constexpr std::array<Dimension, 5> kDimensions = {
        Dimension::GRAMM, Dimension::KILOGRAMM, Dimension::MILLILITER,
        Dimension::LITER, Dimension::PIECE};

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> name_dist(
        0, kProductNames.size() - 1);
    std::uniform_int_distribution<std::size_t> dimension_dist(
        0, kDimensions.size() - 1);
    std::uniform_int_distribution<int> amount_dist(1, 500);

    std::vector<Product> products;
    products.reserve(100);

    for (int i = 0; i < 100; ++i) {
        const auto name = kProductNames.at(name_dist(gen));
        const auto dimension = kDimensions.at(dimension_dist(gen));
        products.emplace_back(name, amount_dist(gen), dimension,
                              MakeRandomDate());
    }

    return products;
}

}  // namespace

int main() {
    std::filesystem::create_directories(
        std::filesystem::path{kDbPath}.parent_path());
    DBManager db(kDbPath);

    const auto products = MakeProducts();
    db.InsertProducts(products);

    return 0;
}
