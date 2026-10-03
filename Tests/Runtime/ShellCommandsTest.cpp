////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ShellCommandsTest.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

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
    test_exit_status_descriptions();
    test_success_requirement();
    test_directory_change();
    test_quotation();
    test_quotation_refusals();
    test_paths_with_spaces();
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
    check(equivalence, "", cmdNoAppend, "foo> \"dir\" 2>&1");
    check(equivalence, "", cmdAppend, "foo>> \"dir\" 2>&1");

    check_semantics("", nullCmd, simpleCmd);
    check_semantics("", simpleCmd, cmdNoAppend);
    check_semantics("", cmdAppend, cmdNoAppend);

    check(equivalence, "Space after digit, before >",  shell_command{"", "foo1", "dir", append_mode::no}, "foo1 > \"dir\" 2>&1");
    check(equivalence, "Space after digit, before >>", shell_command{"", "foo1", "dir", append_mode::yes}, "foo1 >> \"dir\" 2>&1");
  }

  void shell_commands_test::test_exit_status_descriptions()
  {
    auto tabulate{
      [](std::invocable<int> auto describe) {
        constexpr std::array statuses{
          std::numeric_limits<int>::min(), static_cast<int>(0xC0000005u), -2, -1,
          0, 1, 2, 125, 126, 127, 128, 129, 130, 159, 160, 192, 193, 255, 256
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
        return [platform](int status) { return describe_exit_status(status, platform); };
      }
    };

    auto checkDescriptions{
      [this](const reporter& description, std::string_view table, std::string_view fileName) {
        write_to_file(working_materials() /= fileName, table, std::ios_base::out);
        check(equivalence, description, working_materials() /= fileName, predictive_materials() /= fileName);
      }
    };

    checkDescriptions("Exit statuses as described on Windows",
                      tabulate(describerFor(windows_type{})),
                      "WindowsExitStatusDescriptions.txt");

    checkDescriptions("Exit statuses as described on macOS",
                      tabulate(describerFor(macos_type{})),
                      "MacOSExitStatusDescriptions.txt");

    checkDescriptions("Exit statuses as described on Linux",
                      tabulate(describerFor(linux_type{})),
                      "LinuxExitStatusDescriptions.txt");

    checkDescriptions("Exit statuses as described on any other platform",
                      tabulate(describerFor(other_os_type{})),
                      "OtherOSExitStatusDescriptions.txt");

    check(equality,
          "By default, exit statuses are described as on the platform the program is built for",
          tabulate([](int status) { return describe_exit_status(status); }),
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
          std::string{with_windows_v ? "cd /d \"dir\"" : "cd \"dir\""});
  }

  void shell_commands_test::test_quotation()
  {
    auto tabulate{
      [](std::invocable<std::string_view> auto quote) {
        constexpr auto words{
          std::to_array<std::string_view>({
            "plain", "First Last", "", "C:\\Users\\First Last\\build", "C:\\", "trailing\\\\", "$HOME",
            "`date`", "say \"hi\"", "100%", "%PATH%", "line\nfeed", "carriage\rreturn", "a&b|c<d>e^f(g)", "it's!"
          })
        };

        // Line breaks are spelt out, so that each row is one line of the table.
        auto spellLineBreaks{
          [](std::string_view text) { return replace_all(replace_all(text, "\n", "<LF>"), "\r", "<CR>"); }
        };

        auto row{
          [&quote, &spellLineBreaks](std::string_view word) {
            const auto quotation{
              [&quote, word]() -> std::string {
                try
                {
                  return quote(word);
                }
                catch(const std::runtime_error&)
                {
                  return "refused";
                }
              }()
            };

            return std::format("[{}]: {}\n", spellLineBreaks(word), spellLineBreaks(quotation));
          }
        };

        return   words
               | std::views::transform(row)
               | std::views::join
               | std::ranges::to<std::string>();
      }
    };

    auto quoterFor{
      [](auto platform) {
        return [platform](std::string_view word) { return quote_for_shell(word, platform); };
      }
    };

    auto checkQuotations{
      [this](const reporter& description, std::string_view table, std::string_view fileName) {
        write_to_file(working_materials() /= fileName, table, std::ios_base::out);
        check(equivalence, description, working_materials() /= fileName, predictive_materials() /= fileName);
      }
    };

    checkQuotations("Words as quoted for cmd.exe on Windows",
                    tabulate(quoterFor(windows_type{})),
                    "WindowsQuotations.txt");

    checkQuotations("Words as quoted for the shell of macOS",
                    tabulate(quoterFor(macos_type{})),
                    "MacOSQuotations.txt");

    checkQuotations("Words as quoted for the shell of Linux",
                    tabulate(quoterFor(linux_type{})),
                    "LinuxQuotations.txt");

    checkQuotations("Words as quoted for the shell of any other platform",
                    tabulate(quoterFor(other_os_type{})),
                    "OtherOSQuotations.txt");

    check(equality,
          "By default, words are quoted for the shell of the platform the program is built for",
          tabulate([](std::string_view word) { return quote_for_shell(word); }),
          tabulate(quoterFor(platform_constant{})));
  }

  void shell_commands_test::test_quotation_refusals()
  {
    check_exception_thrown<std::runtime_error>(
      "cmd.exe cannot be given a double quote within quotes",
      []() { return quote_for_shell("say \"hi\"", windows_type{}); });

    check_exception_thrown<std::runtime_error>(
      "cmd.exe cannot be given a percent sign within quotes",
      []() { return quote_for_shell("100%", windows_type{}); });

    check_exception_thrown<std::runtime_error>(
      "cmd.exe cannot be given a line break within quotes",
      []() { return quote_for_shell("line\nfeed", windows_type{}); });

  }

  /** Commands run in, and write to, a directory whose name holds a space, and the files they write have
      names which hold one too. The shell splits an unquoted path at the space. If the directory is not
      quoted, the first command fails. If a file is not quoted, the output goes to a file named by the
      first word, and the check of the file's contents fails.
   */
  void shell_commands_test::test_paths_with_spaces()
  {
    namespace fs = std::filesystem;

    const auto spacedDir{scratchpad_materials() /= "Spaced Directory"};
    fs::remove_all(spacedDir);
    fs::create_directories(spacedDir);

    check(equality,
          "A command changes to a directory with a space in its name, and writes to a file named relative to it",
          invoke(cd_cmd(spacedDir) && shell_command{"", "echo moved", "Output File.txt"}),
          0);

    check(equality,
          "The output of the command is in the file within the directory",
          read_to_string(spacedDir / "Output File.txt", std::ios_base::in).value_or(""),
          std::string{"moved\n"});

    const auto program{spacedDir / (with_windows_v ? "Print First Argument.bat" : "Print First Argument")};
    write_to_file(program,
                  with_windows_v ? "@echo %~1\n" : "#!/bin/sh\nprintf '%s\\n' \"$1\"\n",
                  std::ios_base::out);
    fs::permissions(program, fs::perms::owner_exec, fs::perm_options::add);

    const auto argumentFile{spacedDir / "Argument File.txt"};
    const shell_command runProgram{"",
                                   std::format("{} {}", quote_for_shell(program.string()), quote_for_shell("two words")),
                                   argumentFile};

    check(equality,
          "A command which begins with the quoted path of a program runs the program with a quoted argument",
          invoke(runProgram),
          0);

    check(equality,
          "The program receives the argument as one word",
          read_to_string(argumentFile, std::ios_base::in).value_or(""),
          std::string{"two words\n"});
  }
}
