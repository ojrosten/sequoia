////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilties for creating, composing and invoking commandline input.
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

    /** \brief Runs the command, returning its exit status, or -1 if it did not run to completion.

        The spawned process inherits the standard streams and nothing else; in particular it does
        not inherit files the caller happens to have open, which on Windows would otherwise keep
        them undeletable for as long as that process lived.

        The status is the caller's to act on. Ignoring it is rarely right: where only success is
        acceptable, `throw_unless_succeeded` is the usual choice.
     */
    friend int invoke(const shell_command& cmd);
  private:
    std::string m_Command;

    shell_command(std::string cmd, const std::filesystem::path& output, append_mode app);
  };

  /** \brief Changes directory, and on Windows the drive with it.

      Without `/d`, cmd.exe changes the current directory of the target drive but not the current
      drive, so the commands which follow would run wherever the caller was.
   */
  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir);

  /** \brief How a command failed, given a non-zero `status` as returned by `invoke`, phrased to
             follow the command's name.

      A failure is one of:
      - not running to completion, reported by `invoke` as -1; on Windows an exit status of
        0xFFFFFFFF is indistinguishable from it, and is said to be;
      - on Windows, an exit status of 0x80000000 or more, which `invoke`'s `int` makes negative, and
        which is reported in hex, the form in which such statuses are documented;
      - elsewhere, a shell's own "not executable" (126) and "not found" (127), which are said to be;
        cmd.exe exits 1 for a command it cannot find, so on Windows a command not found cannot be
        told from one exiting 1;
      - elsewhere, an exit status above 128, which a shell gives a command killed by a signal, and
        which is said possibly to be one;
      - any other non-zero exit status.
   */
  [[nodiscard]]
  std::string describe_failure(int status);

  /** \brief Throws `std::runtime_error` unless `status`, as returned by `invoke`, is zero.

      The message names `step` and says how it failed, as `describe_failure` does, followed by
      `advice`, which should say what the failure left behind and how to recover.
   */
  void throw_unless_succeeded(int status, std::string_view step, std::string_view advice);

  /** \brief Where the output of a command run from `dir` went: to `output`, resolved against `dir`,
             or to the console if `output` is empty.

      Phrased to follow "the output is", for the advice given to `throw_unless_succeeded`.
   */
  [[nodiscard]]
  std::string where_written(const std::filesystem::path& dir, const std::filesystem::path& output);
}
