////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Implementation details for monotonic sequences.
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeAlgorithms.hpp"

#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace sequoia::maths::impl
{
  template<class C> struct static_storage : std::false_type {};

  template<class T, std::size_t N> struct static_storage<std::array<T, N>> : std::true_type
  {
    constexpr static std::size_t size() noexcept { return N; }
  };

  template<class C>
  struct swap_is_noexcept
    : std::bool_constant<
           std::allocator_traits<typename C::allocator_type>::propagate_on_container_swap::value
        || std::allocator_traits<typename C::allocator_type>::is_always_equal::value
      >
  {};

  template<class C>
    requires (!has_allocator_type_v<C>)
  struct swap_is_noexcept<C> : std::true_type
  {};

  template<class C>
  inline constexpr bool swap_is_noexcept_v{swap_is_noexcept<C>::value};

  template<class T>
  using standard_comparisons
    = std::tuple<std::ranges::less, std::ranges::less_equal, std::ranges::greater, std::ranges::greater_equal,
                 std::less<T>,      std::less_equal<T>,      std::greater<T>,      std::greater_equal<T>,
                 std::less<>,       std::less_equal<>,       std::greater<>,       std::greater_equal<>>;

  /** \brief Whether `Compare` orders values of `T` as one of `<`, `<=`, `>`
             and `>=` does.

      Adding a constant to two such values, without overflow, preserves how
      `Compare` orders them. No compile-time test can establish the property,
      so the trait is true only for the standard library's function objects
      for the four comparisons, over `T` or `void`.
   */
  template<class Compare, class T>
  inline constexpr bool translation_invariant_order_v{
    meta::find_v<standard_comparisons<T>, Compare> < std::tuple_size_v<standard_comparisons<T>>
  };

  /** \brief Whether `T` can represent `x + delta`. */
  template<integer T, integer Delta>
  [[nodiscard]]
  constexpr bool sum_is_representable(const T x, const Delta delta) noexcept
  {
    // Each difference is cast back to its unsigned type, which reduces it
    // modulo the number of that type's values, even where the operands were
    // promoted to int. So both the distance from x to either bound of T and
    // the magnitude of a negative delta are exact.
    using unsigned_t = std::make_unsigned_t<T>;
    constexpr auto maxValue{static_cast<unsigned_t>(std::numeric_limits<T>::max())},
                   lowestValue{static_cast<unsigned_t>(std::numeric_limits<T>::lowest())};
    const auto unsignedX{static_cast<unsigned_t>(x)};

    if(std::cmp_greater_equal(delta, 0))
    {
      const auto roomAbove{static_cast<unsigned_t>(maxValue - unsignedX)};
      return std::cmp_less_equal(delta, roomAbove);
    }

    using unsigned_delta_t = std::make_unsigned_t<Delta>;
    const auto magnitude{static_cast<unsigned_delta_t>(unsigned_delta_t{} - static_cast<unsigned_delta_t>(delta))};
    const auto roomBelow{static_cast<unsigned_t>(unsignedX - lowestValue)};
    return std::cmp_less_equal(magnitude, roomBelow);
  }

  /** \brief Returns `x + delta`, reduced modulo the number of values of `T`.

      The returned value is `x + delta` itself if
      `sum_is_representable(x, delta)`.
   */
  template<integer T, integer Delta>
  [[nodiscard]]
  constexpr T wrapping_sum(const T x, const Delta delta) noexcept
  {
    using unsigned_t = std::make_unsigned_t<T>;
    return static_cast<T>(static_cast<unsigned_t>(static_cast<unsigned_t>(x) + static_cast<unsigned_t>(delta)));
  }
}
