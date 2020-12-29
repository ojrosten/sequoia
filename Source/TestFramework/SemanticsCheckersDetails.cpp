////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "SemanticsCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  [[nodiscard]]
  std::string to_string(const comparison_flavour f)
  {
    switch(f)
    {
    case comparison_flavour::equality:
      return "==";
    case comparison_flavour::inequality:
      return "!=";
    case comparison_flavour::less_than:
      return "<";
    case comparison_flavour::greater_than:
      return ">";
    case comparison_flavour::leq:
      return "<=";
    case comparison_flavour::geq:
      return ">=";
    case comparison_flavour::spaceship:
      return "<=>";
    };

    throw std::logic_error{"Unrecognized case for comparison_flavour"};
  }
}