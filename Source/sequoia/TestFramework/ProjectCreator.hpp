////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for creating new projects, especially from the commandline.
  */

#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <vector>

namespace sequoia::testing
{
  class project_paths;

  enum class build_invocation { no = 0, yes, launch_ide };

  struct project_data
  {
    std::string copyright{};
    std::filesystem::path project_root{};
    indentation code_indent{"\t"};
    build_invocation do_build{build_invocation::launch_ide};
    std::filesystem::path output{};
  };

  void check_indent(const indentation& ind);

  void generate_test_main(std::string_view copyright, const std::filesystem::path& projRoot, indentation codeIndent);

  void generate_build_system_files(const std::filesystem::path& parentProjRoot, const std::filesystem::path& projRoot);

  void init_projects(const project_paths& parentProjectPaths, const std::vector<project_data>& projects, std::ostream& stream);

  [[nodiscard]]
  runtime::shell_command git_first_cmd(const std::filesystem::path& root, const std::filesystem::path& output);

  [[nodiscard]]
  runtime::shell_command launch_cmd(const std::filesystem::path& root, const std::filesystem::path& buildDir);
}
