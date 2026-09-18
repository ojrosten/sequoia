////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities to extract dependencies from the build system.
 */

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** \brief Specifies an object file, and the files read to produce it. */
  struct compilation_record
  {
    std::filesystem::path object{};
    std::vector<std::filesystem::path> inputs{};

    [[nodiscard]]
    friend bool operator==(const compilation_record&, const compilation_record&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const compilation_record& record);
  };

  /** \brief Reads the log Ninja keeps for a build directory.

      Every object the log has ever known is returned, including those the build no longer has,
      in the order the log first names them. Throws `std::runtime_error` if the log cannot be read.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_ninja_deps(const std::filesystem::path& log);

  /** \brief Reads the logs MSBuild associates with a target.

      Every source which wrote an object is returned, ordered by object; each record's inputs are
      the source, then the other files its compilation read, sorted, each once.
      Throws `std::runtime_error` if a log cannot be read, or an object cannot be attributed to
      one source.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_tlogs(const std::filesystem::path& tlogDir);

  /** \brief What a CMake build tree says about itself. */
  struct build_tree
  {
    std::filesystem::path build_directory{};
    std::string generator{};
    std::vector<std::filesystem::path> implicit_include_directories{};
  };

  /** \brief Reads a build tree's description of itself from its `CMakeCache.txt`.

      Throws `std::runtime_error` if the cache cannot be read, or names no generator.
   */
  [[nodiscard]]
  build_tree read_build_tree(const std::filesystem::path& cacheFile);

  /** \brief Every compilation the build currently has, each with its source first among its inputs.

      Throws `std::runtime_error` if the tree was written by a generator whose record of
      dependencies is not understood, has not been built, or has a record which names none of
      the objects the build has.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_compilations(const build_tree& tree, const std::filesystem::path& executable);
}
