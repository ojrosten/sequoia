////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:Commands;

import std;

import :ProjectPaths;
export import sequoia.runtime;

/** \file
    \brief Shell Command specifically for the testing framework
 */

export namespace sequoia::testing
{

  [[nodiscard]]
  runtime::shell_command cmake_cmd(const build_paths& buildPaths,
                                   const std::filesystem::path& output,
                                   const std::optional<std::string>& args = {});

  [[nodiscard]]
  runtime::shell_command build_cmd(const build_paths& buildPaths, const std::filesystem::path& output);

  [[nodiscard]]
  runtime::shell_command build_and_run_cmd(const build_paths& buildPaths, const std::filesystem::path& output);
}
