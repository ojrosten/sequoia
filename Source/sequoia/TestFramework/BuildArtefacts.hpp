////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Readers for what a build leaves behind, from which a translation unit's dependencies are
           known exactly.

    A build already resolved every `#include` and every `import`, macros and conditionals and all,
    and keeps the answer. Ninja's dependency log records what the compiler reported reading for
    each object, and the dyndep file CMake writes for a target records which module interface files
    each object needed and which it produced; Visual Studio's file tracker records both in its
    logs. Either way the dependency graph of an executable is on disk, and is the graph the
    executable was actually built from.
 */

#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** \brief One compilation as the build recorded it: what was read, and which module interface files
             were consumed and produced.

      The same shape whichever generator wrote the build, and whichever of its files is being read:
      Ninja's dependency log fills the inputs and CMake's dyndep file the module files, joined on
      the output; Visual Studio's file tracker logs fill both, joined on the source.
   */
  struct compilation_record
  {
    std::filesystem::path output{};
    std::vector<std::filesystem::path> inputs{};
    std::optional<std::filesystem::path> providedModule{};
    std::vector<std::filesystem::path> requiredModules{};

    [[nodiscard]]
    friend bool operator==(const compilation_record&, const compilation_record&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const compilation_record& record);
  };

  /** \brief Reads Ninja's dependency log, `.ninja_deps`.

      Version 4, which ninja has written since 1.11, is understood; version 3, from ninja 1.10 and
      earlier, is refused rather than guessed at. The log is append-only, so the last record for an
      output is the one which holds, as when ninja reads it; a truncated final record, which ninja
      tolerates and discards, is dropped here too. Paths are as the compiler reported them, so a
      relative one is relative to the build directory. Every output the log has ever known is
      returned, including those of objects the build no longer has; it is for the caller to say
      which are current.

      Throws `std::runtime_error` if the file cannot be opened or is malformed.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_ninja_deps(const std::filesystem::path& log);

  /// Writes a version 4 log which ninja would read - the outputs and inputs, which is all a log holds - so that a build can be described without being performed.
  void write_ninja_deps(const std::filesystem::path& log, std::span<const compilation_record> records);

  /** \brief Reads a dyndep file of version 1.0, which is what CMake writes for a target's C++ modules
             as `CMakeFiles/<target>.dir/CXX.dd`.

      Each statement has the form
      `build <object> [| <module file>]: dyndep [| <module files needed>] [|| <order-only>]`
      with ninja's `$` escapes; the order-only inputs are ignored, and the records carry no inputs.
      Throws `std::runtime_error` if the file cannot be opened or a statement is malformed.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_dyndep(const std::filesystem::path& file);

  /// Writes a dyndep file ninja would read - the outputs and module files, which is all a dyndep holds - the counterpart of `write_ninja_deps`.
  void write_dyndep(const std::filesystem::path& file, std::span<const compilation_record> records);

  /** \brief Reads the tracking logs MSBuild's file tracker leaves beside a Visual Studio build's objects.

      `CL.read.*.tlog` lists, under each source - a line beginning `^`, several sources separated
      by `|` when one invocation compiled them - every file the compiler read; `CL.write.*.tlog`
      lists what it wrote, which is where the object and any `.ifc` are named. Both are UTF-16 with
      a byte order mark, and spell paths in upper case, so each path is put through the filesystem
      to recover its case. A `.ifc` read is a module interface consumed, one written is provided.

      Sources compiled together share their writes, and each object is given to the source whose
      stem or name it bears. What cannot be told apart is refused rather than guessed: throws
      `std::runtime_error` where an object matches no source, or where such a group wrote more
      than one `.ifc`, as it does if a log cannot be opened.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_tlogs(const std::filesystem::path& tlogDir);

  /// Writes `CL.read.1.tlog` and `CL.write.1.tlog` as the tracker would, so that a Visual Studio build can be described without being performed.
  void write_tlogs(const std::filesystem::path& tlogDir, std::span<const compilation_record> records);

  /// What a CMake build tree says about itself, read from its `CMakeCache.txt` and the compiler information beside it.
  struct build_tree
  {
    /// The directory holding `CMakeCache.txt`
    std::filesystem::path root{};

    /// `CMAKE_GENERATOR`
    std::string generator{};

    /// `CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES`: where the standard library and the compiler's own headers live
    std::vector<std::filesystem::path> implicitIncludeDirs{};
  };

  /// Throws `std::runtime_error` if the cache cannot be read.
  [[nodiscard]]
  build_tree read_build_tree(const std::filesystem::path& cacheFile);

  /** \brief Every compilation the build currently has, for the generator which wrote it.

      Ninja: the objects `build.ninja` names, each with its source first among its inputs - which
      the dependency log omits where the compiler is MSVC - and the rest of its inputs from the
      log; the module files from the dyndep files `build.ninja` names. Neither the log's other
      records nor other dyndep files on disk are read: a log is append-only and a tree once
      configured with modules scanned keeps its dyndep files, so each may describe objects and
      edges the build no longer has. Visual Studio: every `*.tlog` directory beneath the root in
      the executable's configuration, which is the name of the directory holding it. Any other
      generator - `Ninja Multi-Config` included, whose statements live elsewhere - throws
      `std::runtime_error`, as does a Ninja tree without its log.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_compilations(const build_tree& tree, const std::filesystem::path& executable);
}
