////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Runtime/ShellCommands.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  runtime::shell_command git_first_cmd(const std::filesystem::path& root, const std::filesystem::path& output);

  [[nodiscard]]
  runtime::shell_command launch_cmd(const std::filesystem::path& root, const std::filesystem::path& buildDir);
}