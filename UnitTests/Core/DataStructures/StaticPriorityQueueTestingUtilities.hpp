#pragma once

#include "UnitTestUtils.hpp"

#include "StaticPriorityQueue.hpp"

namespace sequoia::unit_testing
{  
  template<class T, std::size_t MaxPushes>
  struct equality_checker<data_structures::static_priority_queue<T, MaxPushes>>
  {
    template<class Logger>
    static bool check(Logger& logger, const data_structures::static_priority_queue<T, MaxPushes>& reference, const data_structures::static_priority_queue<T, MaxPushes>& actual, const std::string& description="")
    {
      typename Logger::sentinel s{logger, description};
      bool equal{
        check_equality(logger, reference.empty(), actual.empty(), impl::concat_messages(description, "Inconsistent emptiness"))};

      if(check_equality(logger, reference.size(), actual.size(), impl::concat_messages(description, "Inconsistent size")))
      {
        if(!reference.empty() && !actual.empty())
        {
          if(!check_equality(logger, reference.top(), actual.top(), impl::concat_messages(description, "Inconsistent top element")))
            equal = false;
        }

        if(!check_equality(logger, reference == actual, true, impl::concat_messages(description, "Hidden state")))
          equal = false;
      }
      
      return equal;
    }
  };
}
