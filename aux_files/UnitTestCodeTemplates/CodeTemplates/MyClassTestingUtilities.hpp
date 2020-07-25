////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

//#include "MyClass.hpp"

namespace sequoia::testing
{
  //template<???>
  struct detailed_equality_checker<::my_class/*<???>*/>
  {
    using type = ::my_class/*<???>*/;
    
    template<test_mode Mode>
      static void check(std::string_view description, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // TO DO
    }
  };

  //template<???>
  struct equivalence_checker<::my_class/*<???>*/>
  {
    using type = ::my_class/*<???>*/;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& actual, /*???*/ predictions)
    {
      // TO DO
    }
  };
}
