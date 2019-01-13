////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

//#include "MyType.hpp"

namespace sequoia::unit_testing
{
  //template<???>
  struct details_checker<MyClass/*<???>*/>
  {
    using type = MyClass/*<???>*/;
    
    template<class Logger>
    static void check(Logger& logger, const type& reference, const type& actual, std::string_view description="")
    {
      // TO DO
    }
  };
}
