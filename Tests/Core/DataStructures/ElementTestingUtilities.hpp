////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief An element type for testing how containers construct their elements.
 */

namespace sequoia::testing
{
  /** \brief An element type implicitly constructible from any type.

      Braced construction of a container of this element type, from a container, an initializer list or an allocator,
      selects the initializer-list constructor: each argument becomes an element.
   */
  struct constructible_from_anything
  {
    int value{};

    constexpr constructible_from_anything() = default;

    constexpr constructible_from_anything(const int initialValue)
      : value{initialValue}
    {}

    template<class U>
    constexpr constructible_from_anything(const U&) {}

    [[nodiscard]]
    constexpr bool operator==(const constructible_from_anything&) const noexcept = default;
  };
}
