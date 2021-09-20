////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for recording information from test failures.
 */

#include <string>

namespace sequoia::testing
{
  struct failure_info
  {
    std::size_t check_index;
    std::string message;

    [[nodiscard]]
    friend bool operator==(const failure_info&, const failure_info&) noexcept = default;

    // TO DO: replace with <=> once it lands in libc++
    [[nodiscard]]
    friend bool operator!=(const failure_info&, const failure_info&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const failure_info& lhs, const failure_info& rhs) noexcept
    {
      return (lhs.check_index == rhs.check_index)
           ? lhs.message < rhs.message
           : lhs.check_index < rhs.check_index;
    }

    [[nodiscard]]
    friend bool operator<=(const failure_info& lhs, const failure_info& rhs) noexcept
    {
      return (lhs == rhs) || (lhs < rhs);
    }

    [[nodiscard]]
    friend bool operator>(const failure_info& lhs, const failure_info& rhs) noexcept
    {
      return rhs < lhs;
    }

    [[nodiscard]]
    friend bool operator>=(const failure_info& lhs, const failure_info& rhs) noexcept
    {
      return !(lhs < rhs);
    }
    
    friend std::ostream& operator<<(std::ostream& s, const failure_info& info);

    friend std::istream& operator>>(std::istream& s, failure_info& info);
  };

  using failure_output = std::vector<failure_info>;
}
