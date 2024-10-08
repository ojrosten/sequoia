////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for MoveOnlyTestCore.hpp
*/

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string move_only_message(std::string description)
  {
    return append_lines(description, emphasise("Move-only Semantics"));
  }
}