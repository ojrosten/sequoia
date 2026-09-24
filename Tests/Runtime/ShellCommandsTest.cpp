////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ShellCommandsTest.hpp"

#include <format>

namespace sequoia::testing
{
  using namespace runtime;

  [[nodiscard]]
  std::filesystem::path shell_commands_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void shell_commands_test::run_tests()
  {
    test_composition();
    test_success_requirement();
    test_directory_change();
  }

  void shell_commands_test::test_composition()
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

  void shell_commands_test::test_success_requirement()
  {
    // The message thrown for a status, empty if nothing is thrown
    auto messageFor{
      [](int status) -> std::string {
        try
        {
          throw_unless_succeeded(status, "Doing the thing", "Some advice");
        }
        catch(const std::runtime_error& e)
        {
          return e.what();
        }

        return {};
      }
    };

    auto expected{[](std::string_view outcome) { return std::format("Doing the thing {}\nSome advice\n", outcome); }};

    check(equality, "A zero status", messageFor(0), std::string{});
    check(equality, "A non-zero exit status", messageFor(2), expected("failed with exit status 2"));

    if constexpr(with_windows_v)
    {
      check(equality,
            "No exit status of its own, or 0xFFFFFFFF",
            messageFor(-1),
            expected("did not run to completion, or exited with status 0xFFFFFFFF, which cannot be told apart"));

      check(equality,
            "An exit status of 0x80000000 or more",
            messageFor(static_cast<int>(0xC0000005u)),
            expected("failed with exit status 0xC0000005"));
    }
    else
    {
      check(equality, "No exit status of its own", messageFor(-1), expected("did not run to completion"));

      check(equality,
            "An exit status above 128",
            messageFor(130),
            expected("failed with exit status 130, which may mean it was killed by signal 2"));
    }
  }

  void shell_commands_test::test_directory_change()
  {
    check(equality,
          "Changes drive as well as directory on Windows",
          cd_cmd("dir").string(),
          std::string{with_windows_v ? "cd /d dir" : "cd dir"});
  }
}
