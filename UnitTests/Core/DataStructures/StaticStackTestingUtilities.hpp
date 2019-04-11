////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "StaticStack.hpp"

namespace sequoia::unit_testing
{  
  template<class T, std::size_t MaxDepth>
  struct details_checker<data_structures::static_stack<T, MaxDepth>>
  {
    template<class Logger>
    static void check(Logger& logger, const data_structures::static_stack<T, MaxDepth>& stack, const data_structures::static_stack<T, MaxDepth>& prediction, std::string_view description="")
    {
      check_equality(logger, stack.empty(), prediction.empty(), impl::concat_messages(description, "Inconsistent emptiness"));

      check_equality(logger, stack.size(), prediction.size(), impl::concat_messages(description, "Inconsistent size"));
           
      if(!prediction.empty() && !stack.empty())
      {
        check_equality(logger, stack.top(), prediction.top(), impl::concat_messages(description, "Inconsistent top element"));
      }

      check_equality(logger, prediction == stack, true, impl::concat_messages(description, "Hidden state"));
    }
  };
}
