////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Facility to detect changes on disk and only run the relevant tests.

 */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <iostream>
#include <chrono>
#include <limits>

namespace sequoia::testing
{
  enum class prune_mode { passive, active };

  struct prune_record
  {
    using stamp_type = std::filesystem::file_time_type;

    std::filesystem::path test_path;
    stamp_type time_stamp;

    [[nodiscard]]
    friend auto operator<=>(const prune_record&, const prune_record&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const prune_record& record);

    friend std::istream& operator>>(std::istream& s, prune_record& record);
  };

  /** \brief Whether a unit declaring a module supplies its interface or one of its definitions.

      `interface_unit` is `export module M;` or `export module M:P;`; `implementation_unit` is
      `module M;` or `module M:P;`. Of the four, only `module M;` can be named by no other unit.
      The enumerators are not spelled `interface`/`implementation` because `interface` is a macro in
      the Windows SDK.
   */
  enum class module_role { interface_unit, implementation_unit };

  /** \brief A module declaration, as written. */
  struct module_declaration
  {
    /// `M`, or `M:P` for a partition, with whitespace removed so that `M : P` compares equal.
    std::string name{};
    module_role role{};

    [[nodiscard]]
    friend bool operator==(const module_declaration&, const module_declaration&) noexcept = default;

    /// Renders the declaration as it would have been written, which is how a failure reports it.
    friend std::ostream& operator<<(std::ostream& s, const module_declaration& declaration)
    {
      return s << ((declaration.role == module_role::interface_unit) ? "export module " : "module ")
               << declaration.name
               << ';';
    }

    /// The module a partition belongs to; the whole name for a unit which is not one.
    [[nodiscard]]
    std::string_view primary_name() const noexcept;
  };

  /** \brief The dependencies which the text of a single translation unit declares.

      Lexed rather than preprocessed, so an `#include` behind a false `#if` or inside a string
      literal is reported all the same: over-reporting costs a test which need not have run, where
      under-reporting silently skips one which must. Three spellings do slip through - a header named
      by a macro, one spliced across lines, and one whose tokens a comment separates - and none
      occurs in a tree read today.
   */
  struct source_dependencies
  {
    /// Header names exactly as written, neither resolved against the including file nor filtered.
    std::vector<std::filesystem::path> includes{};

    /** Logical module names, as written. A partition imported from within its own module keeps its
        leading colon - `:P` - because what it abbreviates is only known once the importing unit's
        own declaration has been read.
     */
    std::vector<std::string> imports{};

    /// Absent unless the unit declares a module; `module;` alone introduces no module and is not one.
    std::optional<module_declaration> declaration{};
  };

  /** \brief Lexes the dependencies declared by a translation unit.

      Comments are skipped, as is everything from the first line containing `cutoff`; an empty
      `cutoff` scans to the end.

      A `module` or `import` declaration is recognized only where one may appear: at the start of a
      line, up to horizontal whitespace, and terminated by a semicolon. Neither word is reserved, so
      a line which begins with one and turns out to be something else is put back untouched and
      scanned as ordinary text. `import "header.hpp"` and `import <header>` are header units, and
      are reported as includes, since they are a dependency on a file rather than on a module.
   */
  [[nodiscard]]
  source_dependencies scan_dependencies(std::istream& source, std::string_view cutoff);

  /** \brief The time against which a modification is judged to have happened after the run which
             wrote the prune stamp.

      The stamp is written from a full-resolution clock reading, so whatever comes back is what the
      filesystem was able to store: a whole number of seconds means the implementation truncates -
      libstdc++ does so on macOS, where libc++ records nanoseconds - and a modification made after
      the run began can therefore carry a timestamp up to a second before it. Comparing against the
      stamp itself loses such a change, and loses it in the direction which makes `prune` skip a
      test whose materials really did move. So where the filesystem truncates, the threshold is a
      second earlier than the stamp, which resolves the ambiguous cases as stale.

      One stamp can only separate "sub-second" from "at least a second". A filesystem coarser still
      - two seconds on exFAT, and on some network mounts - reveals itself identically here and
      remains partly exposed; establishing more would mean writing a probe and reading it back,
      which is what the end-to-end test does.

      Where the filesystem records sub-second times the stamp carries them and the threshold is the
      stamp itself. The exception is a stamp landing exactly on a second, which is indistinguishable
      from a truncated one and is treated as coarse: that costs an over-run, never a missed test.
   */
  [[nodiscard]]
  std::filesystem::file_time_type staleness_threshold(std::filesystem::file_time_type stamp);

  [[nodiscard]]
  std::vector<prune_record> read_tests(const std::filesystem::path& file);

  void write_tests(const project_paths& projPaths, const std::filesystem::path& file, std::span<const prune_record> tests);

  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>> tests_to_run(const project_paths& projPaths, std::string_view cutoff);

  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> executedTests,
                          std::span<const std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  void setup_instability_analysis_prune_folder(const project_paths& projPaths);

  void aggregate_instability_analysis_prune_files(const project_paths& projPaths, prune_mode mode, std::filesystem::file_time_type timeStamp, std::size_t numReps);
}
