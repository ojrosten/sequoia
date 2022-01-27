////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Indent.hpp
 */

#include "sequoia/TextProcessing/Indent.hpp"

namespace sequoia
{
  std::string indent(std::string_view sv, indentation ind)
  {
    if(sv.empty()) return {};
    if(ind == no_indent) return std::string{sv};

    std::string str{};
    str.reserve(sv.size());

    std::string::size_type current{};

    while(current < sv.size())
    {
      constexpr auto npos{std::string::npos};
      const auto dist{sv.substr(current).find('\n')};

      const auto count{dist == npos ? npos : dist + 1};
      auto line{sv.substr(current, count == npos ? npos : count)};
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

  [[nodiscard]]
  std::string append_indented(std::string_view sv1, std::string_view sv2, indentation ind)
  {
    std::string str{sv1};
    return append_indented(str, sv2, std::move(ind));
  }

  std::string& tabs_to_spacing(std::string& text, std::string_view spacing)
  {
    if(spacing != "\t")
    {
      constexpr auto npos{std::string::npos};
      std::string::size_type tabPos{};
      while((tabPos = text.find('\t', tabPos)) != npos)
      {
        text.replace(tabPos, 1, spacing);
        tabPos += spacing.size();
      }
    }

    return text;
  }
}
