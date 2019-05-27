////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

//#include "MyClass.hpp"

namespace sequoia::unit_testing
{
  //template<???>
  struct details_checker<::my_class/*<???>*/>
  {
    using type = ::my_class/*<???>*/;
    
    template<class Logger>
    static void check(Logger& logger, const type& reference, const type& actual, std::string_view description)
    {
      // TO DO
    }
  };

  //template<???>
  struct equivalence_checker<::my_class/*<???>*/>
  {
    using type = ::my_class/*<???>*/;
    
    template<class Logger>
    static void check(Logger& logger, const type& actual, /*???*/ refVals, std::string_view description)
    {
      // TO DO
    }
  };
}
