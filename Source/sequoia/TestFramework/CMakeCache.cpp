////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/CMakeCache.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <format>
#include <ranges>

namespace sequoia::testing
{
  cmake_cache::cmake_cache(const build_paths& buildPaths)
    : cmake_cache{buildPaths.cmake_cache_dir() / "CMakeCache.txt"}
  {}

  cmake_cache::cmake_cache(const std::filesystem::path& cacheFile)
  {
    const auto text{read_to_string(cacheFile, std::ios_base::in)};
    if(!text)
      throw std::runtime_error{std::format("cmake_cache: no CMakeCache.txt in {}", cacheFile.parent_path().generic_string())};

    // One entry per line, `NAME:TYPE=VALUE`, among comments opened by `#` or `//` and blank
    // lines. The value may itself contain colons, so the name ends at the first colon before
    // the first `=`. A line may end in `\r`, which is not part of the value.
    for(const auto line : std::views::split(text.value(), '\n'))
    {
      const std::string_view full{line};
      const auto entry{full.ends_with('\r') ? full.substr(0, full.size() - 1) : full};
      if(entry.starts_with('#') || entry.starts_with('/')) continue;

      const auto valuePos{entry.find('=')};
      if(valuePos == std::string_view::npos) continue;

      const auto typePos{entry.find(':')};
      if(typePos >= valuePos) continue;

      m_Variables.emplace(entry.substr(0, typePos), entry.substr(valuePos + 1));
    }
  }

  [[nodiscard]]
  std::optional<std::string> cmake_cache::variable(std::string_view name) const
  {
    const auto found{m_Variables.find(name)};
    return found != m_Variables.end() ? std::optional{found->second} : std::nullopt;
  }

  [[nodiscard]]
  cmake_generator_family cmake_cache::generator_family() const
  {
    const auto generator{variable("CMAKE_GENERATOR")};
    if(!generator)
      throw std::runtime_error{"cmake_cache: the cache does not record CMAKE_GENERATOR"};

    // The Visual Studio generators name their version; the Ninja pair differ by a `Multi-Config` suffix.
    if(generator->starts_with("Visual Studio")) return cmake_generator_family::visual_studio;
    if(generator->starts_with("Ninja"))         return cmake_generator_family::ninja;

    return cmake_generator_family::other;
  }
}
