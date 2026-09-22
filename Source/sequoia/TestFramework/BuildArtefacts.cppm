////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:BuildArtefacts;

import std;

/** \file
    \brief Utilities to extract dependencies from the build system.
 */

export namespace sequoia::testing
{
  /** \brief What a build compiled: the files its record contains, and for each object file, which of
             them were read to produce it.

      `files` holds the files the build's record contains - object files, sources and headers, as the
      build spells them. A `record` holds indices into `files` as a lightweight handle for an object
      file and the inputs used to create it. `files[object_index]` is the object file itself, with the
      `input_indices` indexing into `files` to acquire the files from which the object was produced.

      \pre Each file is recorded only once.

      \pre Every index is less than `files.size()`.
   */
  struct compilations
  {
    using file_index = std::size_t;

    struct record
    {
      file_index object_index{};
      std::vector<file_index> input_indices{};

      [[nodiscard]]
      friend bool operator==(const record&, const record&) noexcept = default;
    };

    std::vector<std::filesystem::path> files{};
    std::vector<record> records{};

    [[nodiscard]]
    friend bool operator==(const compilations&, const compilations&) noexcept = default;
  };

  /** \brief What a CMake build tree says about itself. */
  struct build_tree
  {
    std::filesystem::path build_directory{};
    std::string generator{};
    std::vector<std::filesystem::path> implicit_include_directories{};
  };

  /** \brief Reads a build tree's description of itself from its `CMakeCache.txt`.

      \throws std::runtime_error if the cache cannot be read, or names no generator.
   */
  [[nodiscard]]
  build_tree read_build_tree(const std::filesystem::path& cacheFile);

  /** \brief Every compilation the build currently has, each with its source first among its inputs.

      \throws std::runtime_error if the tree was written by a generator whose record of
      dependencies is not understood, has not been built, or has a record which names none of
      the objects the build has.
   */
  [[nodiscard]]
  compilations read_compilations(const build_tree& tree, const std::filesystem::path& executable);
}
