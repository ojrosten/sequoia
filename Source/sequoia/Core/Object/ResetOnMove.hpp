////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A value reset when moved from.
 */

#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/PlatformSpecific/Macros.hpp"

#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

namespace sequoia::object
{
  /** \brief Wraps a value, defining move semantics such that the moved-from object
      takes the value `Reset`.
   */
  template<std::regular T, T Reset = T{}>
  class reset_on_move
  {
  public:
    using value_type = T;

    constexpr static T reset_value{Reset};

    constexpr reset_on_move() = default;

    SEQUOIA_FORCE_INLINE
    constexpr explicit reset_on_move(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
      : m_Value{std::move(value)}
    {}

    constexpr reset_on_move(const reset_on_move&) = default;

    SEQUOIA_FORCE_INLINE
    constexpr reset_on_move(reset_on_move&& other) noexcept(is_nothrow_exchangeable_v<T, const T&>)
      : m_Value{std::exchange(other.m_Value, Reset)}
    {}

    constexpr reset_on_move& operator=(const reset_on_move&) = default;

    SEQUOIA_FORCE_INLINE
    constexpr reset_on_move& operator=(reset_on_move&& other)
      noexcept(is_nothrow_exchangeable_v<T, const T&> && std::is_nothrow_move_assignable_v<T>)
    {
      m_Value = std::exchange(other.m_Value, Reset);
      return *this;
    }

    [[nodiscard]]
    SEQUOIA_FORCE_INLINE
    constexpr const T& value() const noexcept { return m_Value; }

    [[nodiscard]]
    friend constexpr auto operator<=>(const reset_on_move&, const reset_on_move&) noexcept = default;
  private:
    T m_Value{Reset};
  };
}
