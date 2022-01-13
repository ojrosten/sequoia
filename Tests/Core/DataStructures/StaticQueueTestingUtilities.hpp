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
  struct value_tester<data_structures::static_queue<T, MaxPushes>>
  {
    using type = data_structures::static_queue<T, MaxPushes>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const type& queue, const type& prediction)
    {
      check(equality, "Emptiness incorrect", logger, queue.empty(), prediction.empty());
      check(equality, "Size incorrect", logger, queue.size(), prediction.size());

      if(!prediction.empty() && !queue.empty())
      {
        check(equality, "Front element incorrect", logger, queue.front(), prediction.front());

        check(equality, "Back element incorrect", logger, queue.back(), prediction.back());
      }

      check(equality, "Hidden state incorrect", logger, prediction == queue, true);
      check(equality, "Hidden state, symmetry of operator== incorrect", logger, queue == prediction, true);
    }
  };
}
