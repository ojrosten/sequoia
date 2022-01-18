////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Core/DataStructures/StaticStack.hpp"

namespace sequoia::testing
{
  template<class T, std::size_t MaxDepth>
  struct value_tester<data_structures::static_stack<T, MaxDepth>>
  {
    using type = data_structures::static_stack<T, MaxDepth>;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& stack, const type& prediction)
    {
      check(equality, "Emptiness incorrect", logger, stack.empty(), prediction.empty());
      check(equality, "Size incorrect", logger, stack.size(), prediction.size());

      if(!prediction.empty() && !stack.empty())
      {
        check(flavour, "Top element incorrect", logger, stack.top(), prediction.top());
      }

      check("Hidden state incorrect", logger, prediction == stack);
    }
  };
}
