////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "StaticQueue.hpp"

namespace sequoia::unit_testing
{  
  template<class T, std::size_t MaxPushes>
  struct details_checker<data_structures::static_queue<T, MaxPushes>>
  {
    template<class Logger>
    static void check(Logger& logger, const data_structures::static_queue<T, MaxPushes>& queue, const data_structures::static_queue<T, MaxPushes>& prediction, std::string_view description="")
    {
      check_equality(logger, queue.empty(), prediction.empty(), impl::concat_messages(description, "Inconsistent emptiness"));

      check_equality(logger, queue.size(), prediction.size(), impl::concat_messages(description, "Inconsistent size"));
      
      if(!prediction.empty() && !queue.empty())
      {
        check_equality(logger, queue.front(), prediction.front(), impl::concat_messages(description, "Inconsistent front element"));

        check_equality(logger, queue.back(), prediction.back(), impl::concat_messages(description, "Inconsistent back element"));
      }

      check_equality(logger, prediction == queue, true, impl::concat_messages(description, "Hidden state"));
      check_equality(logger, queue == prediction, true, impl::concat_messages(description, "Hidden state, symmetry of operator=="));
    }
  };
}
