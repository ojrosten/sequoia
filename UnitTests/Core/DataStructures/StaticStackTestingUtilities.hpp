#pragma once

#include "UnitTestUtils.hpp"

#include "StaticStack.hpp"

namespace sequoia::unit_testing
{
  template<class T, std::size_t MaxDepth>
  struct equality_checker<data_structures::static_stack<T, MaxDepth>>
  {
    template<class Logger>
    static bool check(Logger& logger, const data_structures::static_stack<T, MaxDepth>& reference, const data_structures::static_stack<T, MaxDepth>& actual, const std::string& description="")
    {
      typename Logger::sentinel s{logger, description};
      bool equal{check_equality(logger, reference.empty(), actual.empty(), description)};

      return equal;
    }
  };
}
