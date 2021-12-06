////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  template<> struct value_checker<sequoia::testing::failure_info>
  {
    using type = sequoia::testing::failure_info;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check_equality("Check index", logger, actual.check_index, prediction.check_index);
      check_equality("Message", logger, actual.message, prediction.message);
    }
  };

  template<> struct equivalence_checker<sequoia::testing::failure_info>
  {
    using type = sequoia::testing::failure_info;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const std::size_t checkIndex, const std::string& message)
    {
      check_equality("Check index", logger, actual.check_index, checkIndex);
      check_equality("Message", logger, actual.message, message);
    }
  };
}
