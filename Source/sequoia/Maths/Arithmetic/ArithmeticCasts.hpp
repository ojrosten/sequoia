////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Conversion between integer types which throws where a static_cast would silently wrap.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

namespace sequoia::maths
{
  /** \brief Converts `val` to `To` if it is in range, and throws `std::domain_error` otherwise. */
  template<integer To, integer From>
  [[nodiscard]]
  constexpr To checked_conversion_to(From val) noexcept(initializable_from<To, From>)
  {
    if constexpr(!initializable_from<To, From>)
    {
      if(!std::in_range<To>(val))
      {
        constexpr auto lowestVal{std::numeric_limits<To>::lowest()};
        constexpr auto maxVal{std::numeric_limits<To>::max()};
        throw std::domain_error{std::format("Value {} is outside the range [{}, {}] of the target type", val, lowestVal, maxVal)};
      }
    }

    return static_cast<To>(val);
  }
}
