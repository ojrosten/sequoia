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

  [[nodiscard]]
  std::filesystem::path shell_commands_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void shell_commands_test::run_tests()
  {
    using append_mode = shell_command::append_mode;

    shell_command nullCmd{},
                  simpleCmd{"cmd"},
                  cmdNoAppend{"", "foo", "dir", append_mode::no},
                  cmdAppend{"", "foo", "dir", append_mode::yes};

    check(equivalence, "", nullCmd, "");
    check(equivalence, "", simpleCmd, "cmd");
    check(equivalence, "", cmdNoAppend, "foo> dir 2>&1");
    check(equivalence, "", cmdAppend, "foo>> dir 2>&1");

    check_semantics("", nullCmd, simpleCmd);
    check_semantics("", simpleCmd, cmdNoAppend);
    check_semantics("", cmdAppend, cmdNoAppend);

    check(equivalence, "Space after digit, before >",  shell_command{"", "foo1", "dir", append_mode::no}, "foo1 > dir 2>&1");
    check(equivalence, "Space after digit, before >>", shell_command{"", "foo1", "dir", append_mode::yes}, "foo1 >> dir 2>&1");
  }
}
