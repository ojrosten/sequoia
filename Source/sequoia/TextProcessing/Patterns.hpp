////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of functions for finding patterns within text.
 */

#include <string>

namespace sequoia
{
  /// Searchs for matched delimiters; if found returns the positions of the
  /// opening and one past closing.
  [[nodiscard]]
  std::pair<std::string::size_type, std::string::size_type>
  find_matched_delimiters(std::string_view s, std::string::size_type pos={}, char open='(', char close=')');

  [[nodiscard]]
  std::pair<std::string::size_type, std::string::size_type>
  find_sandwiched_text(std::string_view s, std::string_view leftPattern, std::string_view rightPattern, std::string::size_type pos={});
}
