////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

module sequoia.test_framework;

import std;

import sequoia.file_system;
import sequoia.parsing;
import sequoia.platform_specific;
import sequoia.streaming;
import sequoia.text_processing;

/** \file
    \brief Definitions for Commands.hpp
 */

namespace sequoia::testing
{
  using namespace runtime;

  namespace fs = std::filesystem;

  [[nodiscard]]
  shell_command cmake_cmd(const build_paths& buildPaths,
                          const fs::path& output,
                          const std::optional<std::string>& args)
  {
    std::string cmd{std::format("cmake --preset {}", back(buildPaths.cmake_cache_dir()).generic_string())};
    if(args.has_value())
      cmd.append(std::format( "-D EXEC_ARGS {}", args.value()));
    
    return {"Running CMake...",
            cmd,
            output};
  }

  [[nodiscard]]
  shell_command build_cmd(const build_paths& buildPaths, const fs::path& output)
  {
    return {"Building...",
            std::format("cmake --build --preset {}", back(buildPaths.cmake_cache_dir()).generic_string()),
            output};
  }

  [[nodiscard]]
  shell_command build_and_run_cmd(const build_paths& buildPaths, const fs::path& output)
  {
    return {"Building...",
            std::format("cmake --build --preset {} --target run", back(buildPaths.executable_dir()).generic_string()),
            output};
  }
}
