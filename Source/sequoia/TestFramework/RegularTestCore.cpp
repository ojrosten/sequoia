////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string regular_message(std::string_view description)
  {
    return append_lines(description, emphasise("Regular Semantics"));
  }
}