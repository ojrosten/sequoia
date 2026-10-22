////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Conversion of an index to a signed iterator offset.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <cstddef>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

namespace sequoia::utilities
{
  namespace impl
  {
    template<integer Index>
    [[noreturn]]
    constexpr void throw_outside_ptrdiff_range(Index index)
    {
      constexpr auto lowestVal{std::numeric_limits<std::ptrdiff_t>::lowest()};
      constexpr auto maxVal{std::numeric_limits<std::ptrdiff_t>::max()};
      throw std::domain_error{std::format("Value {} is outside the range [{}, {}] of the target type",
                                          index,
                                          lowestVal,
                                          maxVal)};
    }
  }

  /** \brief Converts `index` to `std::ptrdiff_t` without loss.

      \throws std::domain_error if `index` is outside the range of
      `std::ptrdiff_t`.
   */
  template<integer Index>
  [[nodiscard]]
  constexpr std::ptrdiff_t to_difference(Index index) noexcept(initializable_from<std::ptrdiff_t, Index>)
  {
    if constexpr(!initializable_from<std::ptrdiff_t, Index>)
    {
      // The throw is out of line so that clang inlines this check at -O2
      if(!std::in_range<std::ptrdiff_t>(index))
        impl::throw_outside_ptrdiff_range(index);
    }

    return static_cast<std::ptrdiff_t>(index);
  }
}
