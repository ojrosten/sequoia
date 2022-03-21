////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ShellCommandsTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace runtime;
  using namespace std::string_literals;

  [[nodiscard]]
  std::string_view shell_commands_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void shell_commands_false_positive_test::run_tests()
  {
    shell_command x{}, y{"foo"};
    check(equivalence, LINE(""), x, "cmd"s);
    check(equality, LINE(""), x, y);
  }
}