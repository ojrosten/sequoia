////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

#include "StaticPriorityQueue.hpp"

namespace sequoia::testing
{  
  template<class T, std::size_t MaxDepth, class Compare>
  struct detailed_equality_checker<data_structures::static_priority_queue<T, MaxDepth, Compare>>
  {
    using type = data_structures::static_priority_queue<T, MaxDepth, Compare>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& queue, const type& prediction)
    {
      check_equality("Inconsistent emptiness", logger, queue.empty(), prediction.empty());
      check_equality("Inconsistent size", logger, queue.size(), prediction.size());

      if(!prediction.empty() && !queue.empty())
      {
        check_equality("Inconsistent top element", logger, queue.top(), prediction.top());
      }

      check_equality("Inconsistent Hidden state", logger, prediction == queue, true);
      check_equality("Inconsistent Hidden state, symmetry of ==", logger, queue == prediction, true);
    }
  };
}
