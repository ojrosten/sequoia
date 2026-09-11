////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "InvokeInterpreterResolutionFreeTest.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"

#include <fstream>

namespace sequoia::testing
{
  using namespace runtime;

  namespace
  {
    /// The name an impostor would have to take to be mistaken for the interpreter.
    [[nodiscard]]
    std::filesystem::path interpreter_name()
    {
      return with_windows_v ? "cmd.exe" : "sh";
    }

    /// A command which does nothing but hand back a status no failure path also returns.
    [[nodiscard]]
    shell_command exit_with_status_cmd()
    {
      return with_windows_v ? shell_command{"exit /b 7"} : shell_command{"exit 7"};
    }

    /// Removes the impostor however the checks below turn out.
    struct scoped_file
    {
      std::filesystem::path path;

      ~scoped_file()
      {
        std::error_code ec{};
        std::filesystem::remove(path, ec);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path invoke_interpreter_resolution_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void invoke_interpreter_resolution_free_test::run_tests()
  {
    const auto executable{get_project_paths().executable()};
    if(!check("The running executable is known", !executable.empty())) return;

    // The directory holding the running executable is the *first* place Windows looks when asked
    // to resolve a program name, ahead of the current directory and of the system one. It is used
    // here in preference to the current directory because that one is not searched at all when
    // NoDefaultCurrentDirectoryInExePath is set, which would make this check silently vacuous.
    const scoped_file impostor{executable.parent_path() / interpreter_name()};

    if(!check("Nothing already occupies the impostor's path", !std::filesystem::exists(impostor.path))) return;

    {
      std::ofstream stream{impostor.path};
      stream << "Not an executable image.";
    }

    if(!check("The impostor was planted", std::filesystem::exists(impostor.path))) return;

    // Were the interpreter resolved from the command line alone, this is the file which would be
    // run; being no kind of executable, the spawn would fail and `invoke` would report -1.
    check(equality, "The interpreter is found despite the impostor", invoke(exit_with_status_cmd()), 7);
  }
}
