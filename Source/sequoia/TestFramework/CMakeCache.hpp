////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief What a build tree's CMakeCache.txt records about itself.
 */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace sequoia::testing
{
  /** \brief The families of generator whose project files the framework can recognise; every other generator is `other`. */
  enum class cmake_generator_family { visual_studio, ninja, other };

  /** \brief The variables a build tree's CMakeCache.txt records, read once at construction.

      \throws std::runtime_error if the tree has no cache.
   */
  class cmake_cache
  {
  public:
    explicit cmake_cache(const build_paths& buildPaths);

    explicit cmake_cache(const std::filesystem::path& cacheFile);

    /** \brief Disengaged if the cache does not name the variable; an engaged empty string is one it names and leaves empty. */
    [[nodiscard]]
    std::optional<std::string> variable(std::string_view name) const;

    /** \brief From `CMAKE_GENERATOR`, which every cache records. */
    [[nodiscard]]
    cmake_generator_family generator_family() const;

    /** \brief From `CMAKE_HOME_DIRECTORY`, which every cache records: the top-level source directory, the one
               given to `cmake -S`. */
    [[nodiscard]]
    std::filesystem::path source_dir() const;
  private:
    std::map<std::string, std::string, std::ranges::less> m_Variables{};
  };
}
