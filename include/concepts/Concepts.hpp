#pragma once

#include <concepts>
#include <type_traits>

namespace concepts {

template <typename Tv>
concept NumericType = (std::integral<std::remove_cvref_t<Tv>> ||
                       std::floating_point<std::remove_cvref_t<Tv>>) &&
                      requires(Tv a, Tv b) {
                          {
                              +a
                          } -> std::convertible_to<std::remove_cvref_t<Tv>>;
                          {
                              -a
                          } -> std::convertible_to<std::remove_cvref_t<Tv>>;
                          {
                              a + b
                          } -> std::convertible_to<std::remove_cvref_t<Tv>>;
                          {
                              a - b
                          } -> std::convertible_to<std::remove_cvref_t<Tv>>;
                          {
                              a * b
                          } -> std::convertible_to<std::remove_cvref_t<Tv>>;
                          {
                              a / b
                          } -> std::convertible_to<std::remove_cvref_t<Tv>>;
                      };

template <typename TUnit>
concept UnitTag = requires {
    typename TUnit::dimension;
    typename TUnit::ratio;
    requires std::integral<decltype(TUnit::ratio::num)>;
    requires std::integral<decltype(TUnit::ratio::den)>;
};

template <typename TLeftUnit, typename TRightUnit>
concept SameDimension =
    UnitTag<TLeftUnit> && UnitTag<TRightUnit> &&
    std::same_as<typename TLeftUnit::dimension, typename TRightUnit::dimension>;

}  // namespace concepts
