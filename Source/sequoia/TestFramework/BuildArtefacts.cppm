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
  /** \brief A build's compilations: for each, the files read and the object
             file produced.

      `files` holds object files, sources and headers, as spelled by the
      build. A `record` holds indices into `files` as a lightweight handle for
      an object file and the inputs used to create it. `files[object_index]`
      is the object file itself, with the `input_indices` indexing into `files`
      to acquire the files from which the object was produced.
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

  /** \brief Salient properties of a CMake build tree. */
  struct build_tree
  {
    std::filesystem::path build_directory{};
    std::string generator{};
  };

  /** \brief Reads a build tree's salient properties from `cacheFile`.

      \throws std::runtime_error if the cache cannot be read, or names no generator.
   */
  [[nodiscard]]
  build_tree read_build_tree(const std::filesystem::path& cacheFile);

  /** \brief Every compilation the build currently has, each with its source first among its inputs.

      In the result:
      -# No two of `files` are spelled alike.
      -# Every index in `records` is less than `files.size()`.
      -# Every file in `files` is named by one of `records`.

      If the generator is multi-config, the compilations are those of `configuration`. Otherwise
      `configuration` is ignored.

      \throws std::runtime_error if the tree
      -# Was written by a generator whose record of dependencies is not understood;
      -# Was written by a multi-config generator, and `configuration` is empty;
      -# Has not been built;
      -# Was written by a multi-config generator, and holds no record of `configuration`;
      -# Has a record which names none of the objects the build has.
   */
  [[nodiscard]]
  compilations read_compilations(const build_tree& tree, std::string_view configuration);
}
