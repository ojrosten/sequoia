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
  /** \brief The command line `cmake --preset <preset>`, in which `<preset>` is the final component of
             the cmake cache directory of `buildPaths`.
   */
  [[nodiscard]]
  std::string cmake_invocation(const build_paths& buildPaths);

  /** \brief Configures a project, optionally overriding a cache variable.

      \param buildPaths     The project to configure. The preset is the final component of the
                            cmake cache directory of `buildPaths`: a build tree is named after the
                            configure preset that produced it.
      \param output         File to which the command's output is directed.
      \param cacheOverride  Spelled `VAR=VALUE`, as `cmake -D` expects it.
   */
  [[nodiscard]]
  runtime::shell_command cmake_cmd(const build_paths& buildPaths,
                                   const std::filesystem::path& output,
                                   const std::optional<std::string>& cacheOverride = {});

  /** \brief The shell command which builds a project in the configuration in which this library was
             built.

      \param buildPaths  The project to build, by its cmake cache directory.
      \param output      File to which the command's output is directed.
   */
  [[nodiscard]]
  runtime::shell_command build_cmd(const build_paths& buildPaths, const std::filesystem::path& output);

}
