////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularCore.hpp"
#include "Maybe.hpp"

namespace sequoia::testing
{
  template<class T>
  struct detailed_equality_checker<other::functional::maybe<T>>
  {
    using type = other::functional::maybe<T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // TO DO
    }
  };

  template<class T>
  struct equivalence_checker<other::functional::maybe<T>>
  {
    using type = other::functional::maybe<T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const std::optional<T>& prediction)
    {
      // TO DO
    }
  };
}
