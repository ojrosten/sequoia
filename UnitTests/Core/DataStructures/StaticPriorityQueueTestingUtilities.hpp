////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "StaticPriorityQueue.hpp"

namespace sequoia::unit_testing
{  
  template<class T, std::size_t MaxDepth, class Compare>
  struct detailed_equality_checker<data_structures::static_priority_queue<T, MaxDepth, Compare>>
  {
    using type = data_structures::static_priority_queue<T, MaxDepth, Compare>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& queue, const type& prediction)
    {
      check_equality(combine_messages(description, "Inconsistent emptiness"), logger, queue.empty(), prediction.empty());
      check_equality(combine_messages(description, "Inconsistent size"), logger, queue.size(), prediction.size());

      if(!prediction.empty() && !queue.empty())
      {
        check_equality(combine_messages(description, "Inconsistent top element"), logger, queue.top(), prediction.top());
      }

      check_equality(combine_messages(description, "Inconsistent Hidden state"), logger, prediction == queue, true);
      check_equality(combine_messages(description, "Inconsistent Hidden state, symmetry of =="), logger, queue == prediction, true);
    }
  };
}
