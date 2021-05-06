////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "fakeProject/Stuff/Things.hpp"

namespace sequoia::testing
{
  template<> struct detailed_equality_checker<bar::things>
  {
    using type = bar::things;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // TO DO
    }
  };

  template<> struct equivalence_checker<bar::things>
  {
    using type = bar::things;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const double& prediction)
    {
      // TO DO
    }
  };
}
