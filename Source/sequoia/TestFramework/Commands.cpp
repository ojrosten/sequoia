////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Commands.hpp
 */

#include "sequoia/TestFramework/Commands.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

#include <iostream>

namespace sequoia::testing
{
  using namespace runtime;

  namespace fs = std::filesystem;

  namespace
  {
    constexpr auto npos{std::string::npos};

    [[nodiscard]]
    std::string find_property(std::string_view text, std::string_view pattern)
    {
      const auto pos{text.find(pattern)};
      const auto offset{pos < npos ? pos + pattern.size() : pos};

      auto [begin, end]{find_sandwiched_text(text, "=", "\n", offset)};
      return ((begin != npos) && (end != npos)) ? std::string{text.substr(begin, end - begin)} : "";
    };

    [[nodiscard]]
    std::string find_and_set_property(std::string_view text, std::string_view pattern)
    {
      auto property{find_property(text, pattern)};
      return !property.empty() ? std::string{" \"-D"}.append(pattern).append("=").append(property).append("\"") : "";
    }



    [[nodiscard]]
    std::string cmake_extractor(const std::optional<build_paths>& parentBuildPaths,
                                const build_paths& buildPaths)
    {
      const auto buildPathsToUse{parentBuildPaths.has_value() ? parentBuildPaths.value() : buildPaths};

      const auto cmakeCache{buildPathsToUse.cmake_cache()};
      if(!cmakeCache || !fs::exists(*cmakeCache))
        throw std::runtime_error{"Unable to find CMakeCache.txt in " + cmakeCache->parent_path().generic_string()};

      if(const auto optText{read_to_string(*cmakeCache)})
      {
        const auto& text{optText.value()};

        if(auto generator{find_property(text, "CMAKE_GENERATOR")}; !generator.empty())
        {
          std::string genCmd{};

          if(generator != "Unix Makefiles")
          {
            genCmd.append("-G \"").append(generator).append("\"");
            genCmd.append(find_and_set_property(text, "CMAKE_BUILD_TYPE"));
            genCmd.append(find_and_set_property(text, "CMAKE_MAKE_PROGRAM"));
          }

          genCmd.append(find_and_set_property(text, "CMAKE_CXX_COMPILER"));

          if(!genCmd.empty()) return genCmd;
        }

        throw std::runtime_error{"Unable to deduce cmake command from " + cmakeCache->generic_string()};
      }
      else
      {
        throw std::runtime_error{"Unable to read from " + cmakeCache->generic_string()};
      }
    }
  }

  [[nodiscard]]
  shell_command cmake_cmd(const std::optional<build_paths>& parentBuildPaths,
                          const build_paths& buildPaths,
                          const std::filesystem::path& output)
  {
    if(!buildPaths.cmake_cache())
      throw std::runtime_error{"CMakeCache.txt location not specified"};

    auto cmd{std::string{"cmake -S ."}.append(" -B \"").append(buildPaths.cmake_cache()->parent_path().string()).append("\" ")};
    cmd.append(cmake_extractor(parentBuildPaths, buildPaths));

    return {"Running CMake...", cmd, output};
  }

  [[nodiscard]]
  shell_command build_cmd(const build_paths& build, const std::filesystem::path& output)
  {
    if(!build.cmake_cache())
      throw std::runtime_error{"Cannot perform build without a CMakeCache.txt file"};

    const auto cmd{
      [&]() -> shell_command {
        std::string str{"cmake --build . --target TestAll"};
        if(build.executable_dir() != build.cmake_cache()->parent_path())
        {
          const auto subdir{rebase_from(build.executable_dir(), build.cmake_cache()->parent_path())};
          if(!subdir.empty())
            str.append(" --config ").append(subdir.generic_string());
        }

        if constexpr(!with_msvc_v)
        {
          str.append(" -- -j8");
        }

        return {"Building...", str, output};
      }()
    };

    return cd_cmd(build.cmake_cache()->parent_path()) && cmd;
  }
}
