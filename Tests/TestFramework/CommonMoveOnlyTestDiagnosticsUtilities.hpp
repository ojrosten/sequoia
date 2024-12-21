////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"

#include <vector>

namespace sequoia::testing
{
  template<moveonly M>
  struct move_only_beast_value_tester
  {
    using type            = M;
    using equivalent_type = std::vector<typename M::value_type>;

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const equivalent_type&  prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped vector", logger, obtained.x, prediction, advisor);
    }
  };
}
