////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

#include "StaticStack.hpp"

namespace sequoia::testing
{  
  template<class T, std::size_t MaxDepth>
  struct detailed_equality_checker<data_structures::static_stack<T, MaxDepth>>
  {
    using type = data_structures::static_stack<T, MaxDepth>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& stack, const type& prediction)
    {
      check_equality(append_indented(description, "Inconsistent emptiness"), logger, stack.empty(), prediction.empty());

      check_equality(append_indented(description, "Inconsistent size"), logger, stack.size(), prediction.size());
           
      if(!prediction.empty() && !stack.empty())
      {
        check_equality(append_indented(description, "Inconsistent top element"), logger, stack.top(), prediction.top());
      }

      check_equality(append_indented(description, "Hidden state"), logger, prediction == stack, true);
    }
  };
}
