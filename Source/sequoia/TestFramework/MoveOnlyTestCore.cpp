////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

module sequoia.test_framework;

import std;

/** \file
    \brief Definitions for MoveOnlyTestCore.hpp
*/

namespace sequoia::testing
{
  [[nodiscard]]
  std::string move_only_message(std::string description)
  {
    return append_lines(description, emphasise("Move-only Semantics"));
  }
}
