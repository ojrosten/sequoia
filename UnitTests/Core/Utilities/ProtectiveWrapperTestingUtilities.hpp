////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "ProtectiveWrapper.hpp"

namespace sequoia::unit_testing
{
  template<class T>
  struct details_checker<utilities::protective_wrapper<T, std::is_empty_v<T>>>
  {
    template<class Logger>
    static void check(Logger& logger, const utilities::protective_wrapper<T, std::is_empty_v<T>>& reference, const utilities::protective_wrapper<T, std::is_empty_v<T>>& actual, std::string_view description="")
    {
      check(reference.get(), actual.get(), description);
    }
  };
}
