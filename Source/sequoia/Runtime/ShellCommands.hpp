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

  /** \brief The failure of a command for which `invoke` returned `status`, described in a phrase to
             follow the command's name.

      The phrase tells these failures apart:
      -# On Windows, a status of -1: the command either did not run to completion or exited with
         0xFFFFFFFF, and the phrase says that the two cannot be told apart;
      -# On Windows, any other negative status: an exit status of 0x80000000 or more, which the phrase
         gives in hex;
      -# Elsewhere, a negative status: the command did not run to completion;
      -# Elsewhere, a status of 126: the shell's own "not executable";
      -# Elsewhere, a status of 127: the shell's own "not found";
      -# Elsewhere, a status above 128: an exit status which the phrase says may mean that a signal
         killed the command;
      -# Any other status: an exit status.

      cmd.exe exits with 1 for a command it cannot find. So on Windows, a command which is not found
      cannot be told from a command which exits with 1.
   */
  [[nodiscard]]
  std::string describe_failure(int status);

  /** \brief Checks that a status returned by `invoke` is zero.

      \throws std::runtime_error if `status` is not zero. The message is `step`, followed by
      `describe_failure(status)` and then `advice`.
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
