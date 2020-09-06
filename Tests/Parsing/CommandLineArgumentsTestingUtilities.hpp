////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "CommandLineArguments.hpp"
#include "RegularTestCore.hpp"

namespace sequoia::testing
{
  template<>
  struct weak_equivalence_checker<parsing::commandline::operation>
  {
    using type = sequoia::parsing::commandline::operation;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& operation, const type& prediction)
    {
      const bool consistent{((operation.fn != nullptr) && (prediction.fn != nullptr))
          || ((operation.fn == nullptr) && (prediction.fn == nullptr))};
      testing::check("Existence of function objects differs", logger, consistent);
      check_equality("Operation Parameters differ", logger, operation.parameters, prediction.parameters);
    }
  };

  struct function_object
  {
    void operator()(const std::vector<std::string>&) const noexcept {}
  };
}
