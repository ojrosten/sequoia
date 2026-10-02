////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for building, composing and running shell commands.
 */

#include "sequoia/PlatformSpecific/PlatformDiscriminators.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace sequoia::runtime
{
  class shell_command
  {
  public:
    enum class append_mode { no, yes };

    shell_command() = default;

    shell_command(std::string cmd) : m_Command{std::move(cmd)}
    {}

    shell_command(std::string_view preamble, std::string cmd, const std::filesystem::path& output, append_mode app = append_mode::no);

    [[nodiscard]]
    bool empty() const noexcept
    {
      return m_Command.empty();
    }

    [[nodiscard]]
    const std::string& string() const noexcept
    {
      return m_Command;
    }

    [[nodiscard]]
    friend bool operator==(const shell_command&, const shell_command&) noexcept = default;

    [[nodiscard]]
    friend shell_command operator&&(const shell_command& lhs, const shell_command& rhs)
    {
      return rhs.empty() ? lhs :
             lhs.empty() ? rhs :
                           std::string{lhs.m_Command}.append("&&").append(rhs.m_Command);
    }

    [[nodiscard]]
    friend shell_command operator&&(const shell_command& lhs, std::string rhs)
    {
      return lhs && shell_command{std::move(rhs)};
    }

    /** \brief Runs the command, and waits for it to finish.

        \returns
        -# -1, if the command did not run to completion;
        -# Otherwise, the command's exit status. On Windows, an exit status of 0x80000000 or more
           is negative: the `int` keeps the status's 32 bits.

        Apart from the standard streams, the command inherits none of the caller's open files. So
        the caller can delete a file it has open while the command runs, on Windows as elsewhere.

        `invoke` does not act on the status; see `throw_unless_succeeded`.
     */
    friend int invoke(const shell_command& cmd);
  private:
    std::string m_Command;

    shell_command(std::string cmd, const std::filesystem::path& output, append_mode app);
  };

  /** \brief The shell command to change the current directory to `dir`. On Windows, the command also
             changes the current drive to the drive of `dir`.
   */
  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir);

  /** \brief Describes how a command ended, given the `status` which `invoke` returned for it.

      \returns A phrase to follow the command's name, in the words of the platform the program is
      built for.
   */
  [[nodiscard]]
  std::string describe_exit_status(int status);

  /** \brief Describes how a command ended on Windows, given the `status` which `invoke` returned
             for it.

      \returns A phrase to follow the command's name:
      -# If `status` is 0, a phrase saying that the command succeeded;
      -# If `status` is -1, a phrase saying that the command either did not run to completion or
         exited with 0xFFFFFFFF, and that the two cannot be told apart;
      -# If `status` is any other negative value, a phrase giving the exit status in hex;
      -# Otherwise, a phrase giving the exit status.

      cmd.exe exits with 1 for a command it cannot find. So a command which is not found cannot be
      told from a command which exits with 1.
   */
  [[nodiscard]]
  std::string describe_exit_status(int status, windows_type);

  /** \brief Describes how a command ended on macOS, given the `status` which `invoke` returned
             for it.

      \returns A phrase to follow the command's name:
      -# If `status` is 0, a phrase saying that the command succeeded;
      -# If `status` is negative, a phrase saying that the command did not run to completion;
      -# If `status` is 126, a phrase saying that the shell could not execute the command;
      -# If `status` is 127, a phrase saying that the shell could not find the command;
      -# If `status` is from 129 to 159, a phrase giving the exit status, and saying that signal
         number `status` minus 128 may have killed the command;
      -# Otherwise, a phrase giving the exit status.
   */
  [[nodiscard]]
  std::string describe_exit_status(int status, macos_type);

  /** \brief Describes how a command ended on Linux, given the `status` which `invoke` returned
             for it.

      \returns A phrase to follow the command's name:
      -# If `status` is 0, a phrase saying that the command succeeded;
      -# If `status` is negative, a phrase saying that the command did not run to completion;
      -# If `status` is 126, a phrase saying that the shell could not execute the command;
      -# If `status` is 127, a phrase saying that the shell could not find the command;
      -# If `status` is from 129 to 192, a phrase giving the exit status, and saying that signal
         number `status` minus 128 may have killed the command;
      -# Otherwise, a phrase giving the exit status.
   */
  [[nodiscard]]
  std::string describe_exit_status(int status, linux_type);

  /** \brief Describes how a command ended on any other platform, given the `status` which
             `invoke` returned for it.

      \returns A phrase to follow the command's name:
      -# If `status` is 0, a phrase saying that the command succeeded;
      -# If `status` is negative, a phrase saying that the command did not run to completion;
      -# If `status` is 126, a phrase saying that the shell could not execute the command;
      -# If `status` is 127, a phrase saying that the shell could not find the command;
      -# If `status` is from 129 to 255, a phrase giving the exit status, and saying that signal
         number `status` minus 128 may have killed the command;
      -# Otherwise, a phrase giving the exit status.
   */
  [[nodiscard]]
  std::string describe_exit_status(int status, other_os_type);

  /** \brief Checks that a status returned by `invoke` is zero.

      \throws std::runtime_error if `status` is not zero. The message is `step`, followed by
      `describe_exit_status(status)` and then `advice`.
   */
  void throw_unless_succeeded(int status, std::string_view step, std::string_view advice);

  /** \brief Describes where a command run from `dir` wrote its output.

      \returns A phrase which completes the sentence "The output is ...":
      -# If `output` is empty, a phrase naming the console;
      -# Otherwise, a phrase naming the path `output` resolved against `dir`, in generic form.
   */
  [[nodiscard]]
  std::string describe_output_location(const std::filesystem::path& dir, const std::filesystem::path& output);
}
