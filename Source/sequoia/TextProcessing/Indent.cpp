////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TextProcessing/Indent.hpp"

namespace sequoia
{
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
