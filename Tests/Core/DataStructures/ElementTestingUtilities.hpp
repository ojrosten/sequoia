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

#include "sequoia/TestFramework/CoreInfrastructure.hpp"

#include <string>
#include <vector>

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

  template<>
  struct serializer<constructible_from_anything>
  {
    [[nodiscard]]
    static std::string make(const constructible_from_anything& x) { return std::to_string(x.value); }
  };

  template<class T, class U>
  inline constexpr bool equality_expression_valid_v{
    requires(const T& t, const U& u) { t == u; }
  };

  // A hidden-friend operator== would be found by ADL for any type naming this one, and with the converting
  // constructor would make, for example, an iterator comparable with an int
  static_assert(!equality_expression_valid_v<std::vector<constructible_from_anything>::iterator, int>);

  // Likewise a hidden-friend operator<<, which would make a vector of this type serializable as a single element
  static_assert(!serializable<std::vector<constructible_from_anything>>);
}
