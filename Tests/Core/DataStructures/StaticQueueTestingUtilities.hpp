////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Core/DataStructures/StaticQueue.hpp"

namespace sequoia::testing
{
  template<class T, std::size_t MaxPushes>
  struct detailed_equality_checker<data_structures::static_queue<T, MaxPushes>>
  {
    using type = data_structures::static_queue<T, MaxPushes>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& queue, const type& prediction)
    {
      check_equality("Emptiness incorrect", logger, queue.empty(), prediction.empty());
      check_equality("Size incorrect", logger, queue.size(), prediction.size());

      if(!prediction.empty() && !queue.empty())
      {
        check_equality("Front element incorrect", logger, queue.front(), prediction.front());

        check_equality("Back element incorrect", logger, queue.back(), prediction.back());
      }

      check_equality("Hidden state incorrect", logger, prediction == queue, true);
      check_equality("Hidden state, symmetry of operator== incorrect", logger, queue == prediction, true);
    }
  };
}
