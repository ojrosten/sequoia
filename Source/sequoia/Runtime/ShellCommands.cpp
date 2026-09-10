////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

// Not a standard-library header, so import std does not supply it; and it is POSIX, which
// Windows does not have — there std::system yields the child's exit code directly.
#ifndef _WIN32
  #include "sys/wait.h"
#endif

module sequoia.runtime;

import std;

import sequoia.platform_specific;

namespace sequoia::runtime
{
  namespace
  {
    [[nodiscard]]
    int child_exit_status([[maybe_unused]] const int systemResult) noexcept
    {
    #ifdef _WIN32
      return systemResult;
    #else
      return WIFEXITED(systemResult) ? WEXITSTATUS(systemResult) : -1;
    #endif
    }
  }

  shell_command::shell_command(std::string cmd, const std::filesystem::path& output, append_mode app)
    : m_Command{std::move(cmd)}
  {
    if(!output.empty())
    {
      if(!m_Command.empty() && std::isdigit(m_Command.back()))
        m_Command.append(" ");

      m_Command.append(app == append_mode::no ? "> " : ">> ");
      m_Command.append(output.string()).append(" 2>&1");
    }
  }

  shell_command::shell_command(std::string_view preamble, std::string cmd, const std::filesystem::path& output, append_mode app)
  {
    const auto pre{
      [&]() -> shell_command {
        if(!preamble.empty())
        {
          const std::string newline{with_windows_v ? "echo/" : "echo"};
          return shell_command{newline, output, app}
              && shell_command{std::string{"echo "}.append(preamble), output, append_mode::yes}
              && shell_command{newline, output, append_mode::yes};
        }

        return {};
      }()
    };

    *this = pre && shell_command{std::move(cmd), output, !pre.empty() ? append_mode::yes : app};
  }

  int invoke(const shell_command& cmd)
  {
    std::cout << std::flush;
    if(cmd.empty()) return 0;

    return child_exit_status(std::system(cmd.m_Command.data()));
  }

  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir)
  {
    return std::string{"cd "}.append(dir.string());
  }
}
