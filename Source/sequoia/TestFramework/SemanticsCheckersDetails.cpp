////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
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
    \brief Definitions for SemanticsCheckersDetails.hpp
*/

namespace sequoia::testing
{
  [[nodiscard]]
  std::string to_string(const comparison_flavour f)
  {
    switch(f)
    {
    case comparison_flavour::equal:
      return "==";
    case comparison_flavour::not_equal:
      return "!=";
    case comparison_flavour::less_than:
      return "<";
    case comparison_flavour::greater_than:
      return ">";
    case comparison_flavour::less_equal:
      return "<=";
    case comparison_flavour::greater_equal:
      return ">=";
    case comparison_flavour::threeway:
      return "<=>";
    };

    throw std::logic_error{"Unrecognized case for comparison_flavour"};
  }
}
