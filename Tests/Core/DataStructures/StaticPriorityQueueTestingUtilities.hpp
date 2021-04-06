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
  struct detailed_equality_checker<data_structures::static_priority_queue<T, MaxDepth, Compare>>
  {
    using type = data_structures::static_priority_queue<T, MaxDepth, Compare>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& queue, const type& prediction)
    {
      check_equality("Emptiness incorrect", logger, queue.empty(), prediction.empty());
      check_equality("Size incorrect", logger, fixed_width_unsigned_cast(queue.size()), fixed_width_unsigned_cast(prediction.size()));

      if(!prediction.empty() && !queue.empty())
      {
        check_equality("Top element incorrect", logger, queue.top(), prediction.top());
      }

      check_equality("Hidden state incorrect", logger, prediction == queue, true);
      check_equality("Hidden state, symmetry of == incorrect", logger, queue == prediction, true);
    }
  };
}
