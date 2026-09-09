////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Shell Command specifically for the testing framework
 */

#include "sequoia/Runtime/ShellCommands.hpp"

#include <optional>
#include <string>

#include "sequoia/TestFramework/ProjectPaths.hpp"

namespace sequoia::testing
{
  /** \brief Configures a project, optionally overriding a cache variable.

      \param buildPaths     the project to configure; the preset is the final component of its
                            cmake cache directory, which is how the build tree names the
                            configuration that produced it.
      \param output         file to which the command's output is directed.
      \param cacheOverride  spelled `VAR=VALUE`, as `cmake -D` expects it.
   */
  [[nodiscard]]
  runtime::shell_command cmake_cmd(const build_paths& buildPaths,
                                   const std::filesystem::path& output,
                                   const std::optional<std::string>& cacheOverride = {});

  [[nodiscard]]
  runtime::shell_command build_cmd(const build_paths& buildPaths, const std::filesystem::path& output);

}
