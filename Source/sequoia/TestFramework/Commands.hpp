////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Shell Command specifically for the testing framework
 */

#include "sequoia/Runtime/ShellCommands.hpp"

#include "sequoia/TestFramework/ProjectPaths.hpp"

namespace sequoia::testing
{

  [[nodiscard]]
  runtime::shell_command cmake_cmd(const std::optional<build_paths>& parentBuildPaths,
                                   const build_paths& buildPaths,
                                   const std::filesystem::path& output);

  [[nodiscard]]
  runtime::shell_command build_cmd(const build_paths& build, const std::filesystem::path& output);
}
