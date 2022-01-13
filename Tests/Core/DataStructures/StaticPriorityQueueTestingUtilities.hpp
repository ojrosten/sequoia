////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Core/DataStructures/StaticPriorityQueue.hpp"

namespace sequoia::testing
{
  template<class T, std::size_t MaxDepth, class Compare>
  struct value_tester<data_structures::static_priority_queue<T, MaxDepth, Compare>>
  {
    using type = data_structures::static_priority_queue<T, MaxDepth, Compare>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const type& queue, const type& prediction)
    {
      check(equality, "Emptiness incorrect", logger, queue.empty(), prediction.empty());
      check(equality, "Size incorrect", logger, queue.size(), prediction.size());

      if(!prediction.empty() && !queue.empty())
      {
        check(equality, "Top element incorrect", logger, queue.top(), prediction.top());
      }

      check(equality, "Hidden state incorrect", logger, prediction == queue, true);
      check(equality, "Hidden state, symmetry of == incorrect", logger, queue == prediction, true);
    }
  };
}
