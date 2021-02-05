////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"
#include "Container.hpp"

namespace sequoia::testing
{
  template<class T>
  struct detailed_equality_checker<container<T>>
  {
    using type = container<T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // TO DO
    }
  };

  template<class T>
  struct equivalence_checker<container<T>>
  {
    using type = container<T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const std::vector<T>& prediction)
    {
      // TO DO
    }
  };
}
