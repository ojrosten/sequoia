////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Patterns.hpp
 */

#include "sequoia/TextProcessing/Patterns.hpp"

namespace sequoia
{
  [[nodiscard]]
  std::pair<std::string::size_type, std::string::size_type>
  find_matched_delimiters(std::string_view text, const char open, const char close, std::string::size_type pos)
  {
    constexpr auto npos{std::string::npos};

    if(const std::string::size_type openPos{text.find(open, pos)}; openPos != npos)
    {
      int64_t openCount{1};
      pos = openPos+1;
      while(pos < text.size())
      {
        if     (text[pos] == open)  ++openCount;
        else if(text[pos] == close) --openCount;

        ++pos;

        if(!openCount) return {openPos, pos};
      }

      return {openPos, openPos};
    }

    return {npos, npos};
  }

  [[nodiscard]]
  std::pair<std::string::size_type, std::string::size_type>
  find_sandwiched_text(std::string_view s, std::string_view leftPattern, std::string_view rightPattern, std::string::size_type pos)
  {
    constexpr auto npos{std::string::npos};
    const auto start{s.find(leftPattern,pos)};
    if(start == npos) return {npos, npos};

    const auto offset{start + leftPattern.size()};
    return {offset, s.find(rightPattern, offset)};
  }
}
