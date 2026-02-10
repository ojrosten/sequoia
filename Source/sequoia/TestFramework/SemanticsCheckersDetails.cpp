////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for SemanticsCheckersDetails.hpp
*/

#include "sequoia/TestFramework/SemanticsCheckersDetails.hpp"

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
