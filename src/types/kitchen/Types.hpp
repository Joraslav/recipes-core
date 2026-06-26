#pragma once

#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace types {

struct Dates final {
    std::optional<std::chrono::sys_days> manufacture{std::nullopt};
    std::optional<std::chrono::sys_days> expiration{std::nullopt};
};

enum class Dimension : uint8_t { GRAMM, KILOGRAMM, MILLILITER, LITER, PIECE };

/**
 * @brief Product metadata
 * @details Manufacturing and expiration dates are optional
 */
class Product final {
 public:
    explicit Product(std::string_view name, int amount, Dimension dimension,
                     Dates dates = {})
        : name_(name), amount_(amount), dimension_(dimension), dates_(dates) {}

    void SetName(std::string_view name) { name_ = name; }
    void SetManufactureDate(std::chrono::sys_days manufacture_date) {
        dates_.manufacture = manufacture_date;
    }
    void SetExpirationDate(std::chrono::sys_days expiration_date) {
        dates_.expiration = expiration_date;
    }
    void SetAmount(const int& amount) { amount_ = amount; }

    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
    /**
     * @brief Returns name as string_view
     * @warning The view becomes invalid if the object is modified
     */
    [[nodiscard]] std::string_view GetNameView() const noexcept {
        return name_;
    }
    [[nodiscard]] const std::optional<std::chrono::sys_days>&
    GetManufactureDate() const noexcept {
        return dates_.manufacture;
    }
    [[nodiscard]] const std::optional<std::chrono::sys_days>&
    GetExpirationDate() const noexcept {
        return dates_.expiration;
    }
    [[nodiscard]] const int& GetAmount() const noexcept { return amount_; }
    [[nodiscard]] const Dates& GetDates() const noexcept { return dates_; }

    [[nodiscard]] std::string_view GetDimensionInString() const noexcept {
        using namespace std::string_view_literals;
        switch (dimension_) {
            case Dimension::GRAMM:
                return "gr"sv;
            case Dimension::KILOGRAMM:
                return "kg"sv;
            case Dimension::MILLILITER:
                return "ml"sv;
            case Dimension::LITER:
                return "l"sv;
            case Dimension::PIECE:
                return "pc"sv;
            default:
                return ""sv;
        }
    }

    [[nodiscard]] const Dimension& GetDimension() const noexcept {
        return dimension_;
    }

    [[nodiscard]] bool IsFresh() const noexcept {
        if (!dates_.expiration.has_value()) {
            return true;
        }
        const auto today = std::chrono::floor<std::chrono::days>(
            std::chrono::system_clock::now());
        return today <= dates_.expiration.value();
    }

 private:
    std::string name_;
    int amount_;
    Dimension dimension_;
    Dates dates_;
};

/**
 * @brief Recipe metadata
 */
class Recipe final {
 public:
    explicit Recipe(std::string_view name,
                    std::initializer_list<Product> ingredients = {})
        : name_(name), ingredients_(ingredients) {}

    void SetName(std::string_view name) { name_ = name; }
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
    /**
     * @brief Returns name as string_view
     * @warning The view becomes invalid if the object is modified
     */
    [[nodiscard]] std::string_view GetNameView() const noexcept {
        return name_;
    }

    void AddIngredient(Product ingredient) {
        ingredients_.push_back(std::move(ingredient));
    }

    void AddIngredients(std::initializer_list<Product> ingredients) {
        ingredients_.insert(ingredients_.end(), ingredients.begin(),
                            ingredients.end());
    }

    void AddIngredients(std::vector<Product> ingredients) {
        ingredients_.insert(ingredients_.end(),
                            std::make_move_iterator(ingredients.begin()),
                            std::make_move_iterator(ingredients.end()));
    }

    [[nodiscard]] const std::vector<Product>& GetIngredients() const noexcept {
        return ingredients_;
    }

 private:
    std::string name_;
    std::vector<Product> ingredients_;
};

}  // namespace types
