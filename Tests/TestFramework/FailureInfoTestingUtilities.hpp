////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  template<> struct value_tester<sequoia::testing::failure_info>
  {
    using type = sequoia::testing::failure_info;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Check index", logger, actual.check_index, prediction.check_index);
      check(equality, "Message", logger, actual.message, prediction.message);
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::size_t checkIndex, const std::string& message)
    {
      check(equality, "Check index", logger, actual.check_index, checkIndex);
      check(equality, "Message", logger, actual.message, message);
    }
  };
}
