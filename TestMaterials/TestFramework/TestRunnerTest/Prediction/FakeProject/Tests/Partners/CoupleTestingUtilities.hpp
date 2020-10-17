////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"
#include "Couple.hpp"

namespace sequoia::testing
{
  template<class S, class T>
  struct detailed_equality_checker<other::couple<S, T>>
  {
    using type = other::couple<S, T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // TO DO
    }
  };

  template<class S, class T>
  struct equivalence_checker<other::couple<S, T>>
  {
    using type = other::couple<S, T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const S& prediction_0, const T& prediction_1)
    {
      // TO DO
    }
  };
}
