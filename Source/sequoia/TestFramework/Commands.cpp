////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/Commands.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"

#include <format>
#include <string_view>

namespace sequoia::testing
{
  using namespace runtime;

  namespace fs = std::filesystem;

  namespace
  {
    /// This library's configuration, CMake's `$<CONFIG>`: empty for a single-config build given no build type
    constexpr std::string_view library_configuration{SEQUOIA_BUILD_CONFIGURATION};
  }

  [[nodiscard]]
  shell_command cmake_cmd(const build_paths& buildPaths,
                          const fs::path& output,
                          const std::optional<std::string>& cacheOverride)
  {
    const auto preset{back(buildPaths.cmake_cache_dir()).generic_string()};
    const auto cmd{
      cacheOverride ? std::format("cmake --preset {} -D {}", preset, cacheOverride.value())
                    : std::format("cmake --preset {}", preset)
    };

    return {"Running CMake...", cmd, output};
  }

  [[nodiscard]]
  shell_command build_cmd(const build_paths& buildPaths, const fs::path& output)
  {
    const auto cacheDir{buildPaths.cmake_cache_dir().generic_string()};
    const auto cmd{
      library_configuration.empty() ? std::format("cmake --build {}", cacheDir)
                                    : std::format("cmake --build {} --config {}", cacheDir, library_configuration)
    };

    return {"Building...", cmd, output};
  }
}
