////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilties for creating, composing and invoking commandline input.
 */

#include <filesystem>
#include <optional>

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

    friend void invoke(const shell_command& cmd);
  private:
    std::string m_Command;

    shell_command(std::string cmd, const std::filesystem::path& output, append_mode app);
  };

  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir);
}
