////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ShellCommandsTest.hpp"

namespace sequoia::testing
{
  using namespace runtime;
  using namespace std::string_literals;

  [[nodiscard]]
  std::string_view shell_commands_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void shell_commands_test::run_tests()
  {
    shell_command x{}, y{"cmd"};
    check(equivalence, LINE(""), x, ""s);
    check(equivalence,LINE(""), y, "cmd"s);

    check_semantics(LINE(""), x, y);
  }
}
