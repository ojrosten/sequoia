////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"
#include "Foo.hpp"

namespace sequoia::testing
{
  template<class T>
  struct detailed_equality_checker<bar::baz::foo<T>>
  {
    using type = bar::baz::foo<T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      // TO DO
    }
  };

  template<class T>
  struct equivalence_checker<bar::baz::foo<T>>
  {
    using type = bar::baz::foo<T>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const T& prediction)
    {
      // TO DO
    }
  };
}
