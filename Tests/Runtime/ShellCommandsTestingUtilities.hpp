////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"

namespace sequoia::testing
{
  template<> struct value_tester<runtime::shell_command>
  {
    using type = runtime::shell_command;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Description", logger, actual.string(), prediction.string());
    }
    
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::string& prediction)
    {
      check(equality, "Wrapped command", logger, actual.string(), prediction);
    }
  };
}
