////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "StaticPriorityQueue.hpp"

namespace sequoia::unit_testing
{  
  template<class T, std::size_t MaxDepth, class Compare>
  struct details_checker<data_structures::static_priority_queue<T, MaxDepth, Compare>>
  {
    using type = data_structures::static_priority_queue<T, MaxDepth, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& queue, const type& prediction, std::string_view description="")
    {
      check_equality(logger, queue.empty(), prediction.empty(), impl::concat_messages(description, "Inconsistent emptiness"));
      check_equality(logger, queue.size(), prediction.size(), impl::concat_messages(description, "Inconsistent size"));

      if(!prediction.empty() && !queue.empty())
      {
        check_equality(logger, queue.top(), prediction.top(), impl::concat_messages(description, "Inconsistent top element"));
      }

      check_equality(logger, prediction == queue, true, impl::concat_messages(description, "Inconsistent Hidden state"));
      check_equality(logger, queue == prediction, true, impl::concat_messages(description, "Inconsistent Hidden state, symmetry of =="));
    }
  };
}
