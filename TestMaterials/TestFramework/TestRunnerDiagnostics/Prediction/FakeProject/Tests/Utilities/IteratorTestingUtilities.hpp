////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"
#include "Iterator.hpp"

namespace sequoia::testing
{
  struct detailed_equality_checker<utilities::iterator>
  {
    using type = utilities::iterator;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // TO DO
    }
  };

  struct equivalence_checker<utilities::iterator>
  {
    using type = utilities::iterator;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, int* prediction)
    {
      // TO DO
    }
  };
}
