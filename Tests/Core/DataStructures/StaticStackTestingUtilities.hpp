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

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const type& stack, const type& prediction)
    {
      check_equality("Emptiness incorrect", logger, stack.empty(), prediction.empty());
      check_equality("Size incorrect", logger, stack.size(), prediction.size());

      if(!prediction.empty() && !stack.empty())
      {
        check_equality("Top element incorrect", logger, stack.top(), prediction.top());
      }

      check_equality("Hidden state incorrect", logger, prediction == stack, true);
    }
  };
}
