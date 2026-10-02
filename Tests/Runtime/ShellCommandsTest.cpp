////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ShellCommandsTest.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <array>
#include <concepts>
#include <format>
#include <limits>
#include <ranges>

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
    test_failure_descriptions();
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

  void shell_commands_test::test_failure_descriptions()
  {
    auto tabulate{
      [](std::invocable<int> auto describe) {
        constexpr std::array statuses{
          std::numeric_limits<int>::min(), static_cast<int>(0xC0000005u), -2, -1,
          0, 1, 2, 125, 126, 127, 128, 129, 130, 255, 256
        };

        auto row{[&describe](int status) { return std::format("{}: {}\n", status, describe(status)); }};

        return   statuses
               | std::views::transform(row)
               | std::views::join
               | std::ranges::to<std::string>();
      }
    };

    auto describerFor{
      [](auto platform) {
        return [platform](int status) { return describe_failure(status, platform); };
      }
    };

    auto checkDescriptions{
      [this](const reporter& description, std::string_view table, std::string_view fileName) {
        write_to_file(working_materials() /= fileName, table, std::ios_base::out);
        check(equivalence, description, working_materials() /= fileName, predictive_materials() /= fileName);
      }
    };

    checkDescriptions("Failures as described on Windows",
                      tabulate(describerFor(windows_type{})),
                      "WindowsFailureDescriptions.txt");

    checkDescriptions("Failures as described on macOS",
                      tabulate(describerFor(macos_type{})),
                      "MacOSFailureDescriptions.txt");

    check(equality,
          "Linux describes failures as macOS does",
          tabulate(describerFor(linux_type{})),
          tabulate(describerFor(macos_type{})));

    check(equality,
          "Any other platform describes failures as macOS does",
          tabulate(describerFor(other_os_type{})),
          tabulate(describerFor(macos_type{})));

    check(equality,
          "By default, failures are described as on the platform the program is built for",
          tabulate([](int status) { return describe_failure(status); }),
          tabulate(describerFor(platform_constant{})));
  }

  void shell_commands_test::test_success_requirement()
  {
    // A zero status must not throw. A throw here ends the test with a critical failure.
    throw_unless_succeeded(0, "Succeeding", "No advice");

    check_exception_thrown<std::runtime_error>(
      "The message names the step, says how the step failed, then gives the advice",
      []() { throw_unless_succeeded(2, "Doing the thing", "Some advice"); });
  }

  void shell_commands_test::test_directory_change()
  {
    check(equality,
          "Changes drive as well as directory on Windows",
          cd_cmd("dir").string(),
          std::string{with_windows_v ? "cd /d dir" : "cd dir"});
  }
}
