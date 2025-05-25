////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeCheckers.hpp"

#include <vector>

namespace sequoia::testing
{
  template<class T>
  struct beast_equivalence_tester
  {
    using type            = T;
    using equivalent_type = std::vector<typename T::value_type, typename T::allocator_type>;

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const equivalent_type& prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped vector", logger, obtained.x, prediction, advisor);
    }
  };
  
  enum class enable_serialization : bool {no, yes};
}
