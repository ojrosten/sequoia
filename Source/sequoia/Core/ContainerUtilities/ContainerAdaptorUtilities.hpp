////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for container adaptors, such as a queue or a stack.
 */

#include <concepts>

namespace sequoia::utilities
{
  /** \brief Whether `front()` on a const `Container` returns a const reference
             to the container's value type.
   */
  template<class Container>
  inline constexpr bool gives_references_to_elements_v{
    requires(const Container& container) {
      { container.front() } -> std::same_as<const typename Container::value_type&>;
    }
  };

  template<class Adaptor>
  inline constexpr bool has_front_v{requires(const Adaptor& adaptor) { adaptor.front(); }};

  template<class Adaptor>
  inline constexpr bool has_top_v{requires(const Adaptor& adaptor) { adaptor.top(); }};

  template<class Adaptor>
  inline constexpr bool has_pop_v{requires(Adaptor& adaptor) { adaptor.pop(); }};

  /// Returns the element of `adaptor` which the next pop removes
  template<class Adaptor>
    requires has_pop_v<Adaptor> && (has_front_v<Adaptor> != has_top_v<Adaptor>)
  [[nodiscard]]
  constexpr decltype(auto) element_popped_next(const Adaptor& adaptor)
  {
    if constexpr(has_front_v<Adaptor>)
      return adaptor.front();
    else
      return adaptor.top();
  }
}
