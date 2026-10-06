////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

#ifndef SEQUOIA_BUILD_CONFIGURATION
  #warning "SEQUOIA_BUILD_CONFIGURATION is not defined, so build_cmd omits --config; CMake defines it for the sequoia target"
  #define SEQUOIA_BUILD_CONFIGURATION ""
#endif

module sequoia.test_framework;

import std;

import sequoia.file_system;
import sequoia.parsing;
import sequoia.platform_specific;
import sequoia.streaming;
import sequoia.text_processing;

namespace sequoia::testing
{
  using namespace runtime;

  namespace fs = std::filesystem;

  namespace
  {
    /// The configuration in which this library was built, as CMake's `$<CONFIG>` gives it. The configuration
    /// is empty for a single-config build given no build type.
    constexpr std::string_view library_configuration{SEQUOIA_BUILD_CONFIGURATION};
  }

  [[nodiscard]]
  std::string cmake_invocation(const build_paths& buildPaths)
  {
    return std::format("cmake --preset {}", quote_for_shell(back(buildPaths.cmake_cache_dir()).generic_string()));
  }

  [[nodiscard]]
  shell_command cmake_cmd(const build_paths& buildPaths,
                          const fs::path& output,
                          const std::optional<std::string>& cacheOverride)
  {
    auto cmd{cmake_invocation(buildPaths)};
    if(cacheOverride) cmd.append(" -D ").append(cacheOverride.value());

    return {"Running CMake...", cmd, output};
  }

  [[nodiscard]]
  shell_command build_cmd(const build_paths& buildPaths, const fs::path& output)
  {
    auto cmd{std::format("cmake --build {}", quote_for_shell(buildPaths.cmake_cache_dir().generic_string()))};
    if(!library_configuration.empty())
      cmd.append(std::format(" --config {}", library_configuration));

    return {"Building...", cmd, output};
  }
}
