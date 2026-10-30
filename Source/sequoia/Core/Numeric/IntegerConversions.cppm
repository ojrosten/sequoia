////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

// assert is a macro, so import std does not supply it
#include <cassert>

export module sequoia.core.numeric:IntegerConversions;

import std;

export import sequoia.core.meta;

/** \file
    \brief Conversion between integer types, checked by throwing or by a precondition, where a static_cast would silently wrap.
 */

export namespace sequoia::numeric
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

  /** \brief Converts `val` to `To`.

      \pre `std::in_range<To>(val)`
   */
  template<integer To, integer From>
  [[nodiscard]]
  constexpr To unchecked_conversion_to(From val)
  {
    assert(std::in_range<To>(val));

    return static_cast<To>(val);
  }
}
