#pragma once

#include "UnitTestUtils.hpp"

#include "SharingPolicies.hpp"

namespace sequoia::unit_testing
{
  template<class T> struct type_to_string<data_sharing::shared<T>>
  {
    static std::string str() { return "SHARED DATA"; }
  };

  template<class T> struct type_to_string<data_sharing::independent<T>>
  {
    static std::string str() { return "UNSHARED DATA"; }
  };
}
