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

#include <type_traits>

namespace sequoia::utilities
{
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

  /** \brief Pops `adaptor`, and returns a copy of the element the pop removes.

      If copying the element throws, `adaptor` is unchanged.
   */
  template<class Adaptor>
    requires requires(Adaptor& adaptor) { element_popped_next(adaptor); }
          && std::is_nothrow_move_constructible_v<typename Adaptor::value_type>
  [[nodiscard]]
  constexpr typename Adaptor::value_type extract_next(Adaptor& adaptor)
  {
    // Not const, so that the return moves `element`: a copy there could throw
    // after the pop
    typename Adaptor::value_type element{element_popped_next(adaptor)};
    adaptor.pop();
    return element;
  }
}
