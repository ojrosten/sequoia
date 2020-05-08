////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

//#include "Iterator.hpp"

namespace sequoia::unit_testing
{
  //template<???>
  struct details_checker<utilities::iterator/*<???>*/>
  {
    using type = utilities::iterator/*<???>*/;
    
    template<test_mode Mode>
    static void check(unit_test_logger<Mode>& logger, const type& reference, const type& actual, std::string_view description)
    {
      // TO DO
    }
  };

  //template<???>
  struct equivalence_checker<utilities::iterator/*<???>*/>
  {
    using type = utilities::iterator/*<???>*/;
    
    template<test_mode Mode>
    static void check(unit_test_logger<Mode>& logger, const type& actual, /*???*/ refVals, std::string_view description)
    {
      // TO DO
    }
  };
}
