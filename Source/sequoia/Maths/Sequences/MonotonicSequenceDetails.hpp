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

#include <array>
#include <functional>
#include <limits>
#include <memory>
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

  /** \brief True only for comparisons known to preserve how they order two
             integers when a constant is added to both, without overflow.

      No compile-time test can establish the property, so the trait is false
      for some comparisons that have it.
   */
  template<class Compare>
  inline constexpr bool translation_invariant_order_v{
    std::same_as<Compare, std::ranges::less> || std::same_as<Compare, std::ranges::greater>
  };

  /** \brief Whether `T` can represent `x + delta`. */
  template<integer T, integer Delta>
    requires std::signed_integral<Delta>
  [[nodiscard]]
  constexpr bool sum_is_representable(const T x, const Delta delta) noexcept
  {
    // The distances from x to T's bounds are exact in T's unsigned type, once
    // cast back to it after any promotion to int.
    using unsigned_t = std::make_unsigned_t<T>;
    constexpr auto maxValue{static_cast<unsigned_t>(std::numeric_limits<T>::max())},
                   lowestValue{static_cast<unsigned_t>(std::numeric_limits<T>::lowest())};
    const auto unsignedX{static_cast<unsigned_t>(x)};
    const auto roomAbove{static_cast<unsigned_t>(maxValue - unsignedX)},
               roomBelow{static_cast<unsigned_t>(unsignedX - lowestValue)};

    // For a negative delta, -(delta + 1) < roomBelow is -delta <= roomBelow,
    // without negating the lowest value of Delta.
    return delta >= 0 ? std::cmp_less_equal(delta, roomAbove) : std::cmp_less(-(delta + 1), roomBelow);
  }

  /** \brief Returns `x + delta`, reduced modulo the number of values of `T`.

      The returned value is `x + delta` itself if
      `sum_is_representable(x, delta)`.
   */
  template<integer T, integer Delta>
    requires std::signed_integral<Delta>
  [[nodiscard]]
  constexpr T wrapping_sum(const T x, const Delta delta) noexcept
  {
    using unsigned_t = std::make_unsigned_t<T>;
    return static_cast<T>(static_cast<unsigned_t>(static_cast<unsigned_t>(x) + static_cast<unsigned_t>(delta)));
  }
}
