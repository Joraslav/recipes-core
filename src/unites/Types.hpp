#pragma once

#include "concepts/Concepts.hpp"

#include <ratio>
#include <type_traits>

namespace unites {

struct MassDimension final {};
struct VolumeDimension final {};

struct Gram final {
    using dimension = MassDimension;
    using ratio = std::ratio<1>;
};

struct Kilogram final {
    using dimension = MassDimension;
    using ratio = std::ratio<1000>;
};

struct Milliliter final {
    using dimension = VolumeDimension;
    using ratio = std::ratio<1>;
};

struct Liter final {
    using dimension = VolumeDimension;
    using ratio = std::ratio<1000>;
};

template <concepts::UnitTag TLeftUnit, concepts::UnitTag TRightUnit>
    requires concepts::SameDimension<TLeftUnit, TRightUnit>
using SmallerUnit = std::conditional_t<
    std::ratio_less_v<typename TLeftUnit::ratio, typename TRightUnit::ratio>,
    TLeftUnit, TRightUnit>;

template <concepts::UnitTag TUnit>
class Unites final {
 public:
    using value_type = double;
    using unit = TUnit;

    constexpr explicit Unites(double value) noexcept : value_(value) {}

    template <concepts::SameDimension<TUnit> TOtherUnit>
    constexpr explicit Unites(const Unites<TOtherUnit>& other) noexcept
        : value_(other.template As<TUnit>()) {}

    [[nodiscard]] constexpr double Value() const noexcept { return value_; }

    template <concepts::UnitTag TTargetUnit>
        requires concepts::SameDimension<TUnit, TTargetUnit>
    [[nodiscard]] constexpr auto As() const noexcept -> double {
        const auto base_value = (static_cast<double>(value_) *
                                 static_cast<double>(TUnit::ratio::num)) /
                                static_cast<double>(TUnit::ratio::den);
        return (base_value * static_cast<double>(TTargetUnit::ratio::den)) /
               static_cast<double>(TTargetUnit::ratio::num);
    }

    template <concepts::SameDimension<TUnit> TOtherUnit>
    [[nodiscard]] constexpr auto operator+(
        const Unites<TOtherUnit>& other) const noexcept {
        using result_unit = SmallerUnit<TUnit, TOtherUnit>;
        return Unites<result_unit>{this->template As<result_unit>() +
                                   other.template As<result_unit>()};
    }

    template <concepts::SameDimension<TUnit> TOtherUnit>
    [[nodiscard]] constexpr auto operator-(
        const Unites<TOtherUnit>& other) const noexcept {
        using result_unit = SmallerUnit<TUnit, TOtherUnit>;
        return Unites<result_unit>{this->template As<result_unit>() -
                                   other.template As<result_unit>()};
    }

    template <concepts::NumericType TScalar>
    [[nodiscard]] constexpr auto operator*(TScalar scalar) const noexcept {
        return Unites<TUnit>{value_ * static_cast<double>(scalar)};
    }

    template <concepts::NumericType TScalar>
    [[nodiscard]] constexpr auto operator/(TScalar scalar) const noexcept {
        return Unites<TUnit>{value_ / static_cast<double>(scalar)};
    }

 private:
    double value_;
};

template <concepts::UnitTag TUnit, concepts::NumericType TScalar>
[[nodiscard]] constexpr auto operator*(TScalar scalar,
                                       const Unites<TUnit>& value) noexcept {
    return value * scalar;
}

[[nodiscard]] constexpr auto operator""_ml(unsigned long long value) noexcept {
    return Unites<Milliliter>{static_cast<double>(value)};
}

[[nodiscard]] constexpr auto operator""_ml(long double value) noexcept {
    return Unites<Milliliter>{static_cast<double>(value)};
}

[[nodiscard]] constexpr auto operator""_l(unsigned long long value) noexcept {
    return Unites<Liter>{static_cast<double>(value)};
}

[[nodiscard]] constexpr auto operator""_l(long double value) noexcept {
    return Unites<Liter>{static_cast<double>(value)};
}

[[nodiscard]] constexpr auto operator""_g(unsigned long long value) noexcept {
    return Unites<Gram>{static_cast<double>(value)};
}

[[nodiscard]] constexpr auto operator""_g(long double value) noexcept {
    return Unites<Gram>{static_cast<double>(value)};
}

[[nodiscard]] constexpr auto operator""_kg(unsigned long long value) noexcept {
    return Unites<Kilogram>{static_cast<double>(value)};
}

[[nodiscard]] constexpr auto operator""_kg(long double value) noexcept {
    return Unites<Kilogram>{static_cast<double>(value)};
}

}  // namespace unites
