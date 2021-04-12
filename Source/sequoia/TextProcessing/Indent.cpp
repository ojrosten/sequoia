////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Output.hpp.
 */

#include "sequoia/TextProcessing/Indent.hpp"

namespace sequoia
{
  std::string indent(std::string_view s, indentation ind)
  {
    if(s.empty()) return {};
    if(ind == no_indent) return std::string{s};

    std::string str{};
    str.reserve(s.size());

    std::string::size_type current{};

    while(current < s.size())
    {
      constexpr auto npos{std::string::npos};
      const auto dist{s.substr(current).find('\n')};

      const auto count{dist == npos ? npos : dist + 1};
      auto line{s.substr(current, count == npos ? npos : count)};
      if(line.find_first_not_of('\n') != npos)
        str.append(ind);

      str.append(line);

      current = (count == npos) ? npos : current + count;
    }

    return str;
  }

  std::string& append_indented(std::string& s1, std::string_view s2, indentation ind)
  {
    if(!s2.empty())
    {
      if(s1.empty())
      {
        s1 = s2;
      }
      else
      {
        s1.append("\n").append(indent(s2, ind));
      }
    }

    return s1;
  }
}
