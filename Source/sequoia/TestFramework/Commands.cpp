////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/Commands.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"

#include <format>

namespace sequoia::testing
{
  using namespace runtime;

  namespace fs = std::filesystem;

  [[nodiscard]]
  std::string cmake_invocation(const build_paths& buildPaths)
  {
    return std::format("cmake --preset {}", back(buildPaths.cmake_cache_dir()).generic_string());
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
    return {"Building...",
            std::format("cmake --build --preset {}", back(buildPaths.cmake_cache_dir()).generic_string()),
            output};
  }
}
