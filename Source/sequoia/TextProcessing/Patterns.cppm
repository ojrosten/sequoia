////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.text_processing:Patterns;

import std;

/** \file
    \brief A collection of functions for finding patterns within text.
 */

export namespace sequoia
{
  /** \brief Searches `s`, from `pos`, for the first `open` and the `close` that balances it.
      \returns
      -# The positions of that `open` and one past its `close`;
      -# The position of that `open` twice, if no `close` balances it;
      -# `npos` twice, if there is no `open`.
   */
  [[nodiscard]]
  std::pair<std::string::size_type, std::string::size_type>
  find_matched_delimiters(std::string_view s, char open, char close, std::string::size_type pos={});

  [[nodiscard]]
  std::pair<std::string::size_type, std::string::size_type>
  find_sandwiched_text(std::string_view s, std::string_view leftPattern, std::string_view rightPattern, std::string::size_type pos={});
}
