#pragma once

#include "UnitTestUtils.hpp"

#include "StaticStack.hpp"

namespace sequoia::unit_testing
{  
  template<class T, std::size_t MaxDepth>
  struct equality_checker<data_structures::static_stack<T, MaxDepth>>
  {
    template<class Logger>
    static void check(Logger& logger, const data_structures::static_stack<T, MaxDepth>& reference, const data_structures::static_stack<T, MaxDepth>& actual, std::string_view description="")
    {
      check_equality(logger, reference.empty(), actual.empty(), impl::concat_messages(description, "Inconsistent emptiness"));

      check_equality(logger, reference.size(), actual.size(), impl::concat_messages(description, "Inconsistent size"));
           
      if(!reference.empty() && !actual.empty())
      {
        check_equality(logger, reference.top(), actual.top(), impl::concat_messages(description, "Inconsistent top element"));
      }

      check_equality(logger, reference == actual, true, impl::concat_messages(description, "Hidden state"));
    }
  };
}
