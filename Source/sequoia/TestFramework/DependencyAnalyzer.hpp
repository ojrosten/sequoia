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

#include <chrono>
#include <iostream>
#include <limits>
#include <span>

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

  /** \brief Reads the module declaration with which a translation unit opens, if it has one.

      Only the preamble is read: a module declaration precedes every other declaration in the
      unit, with nothing before it but the global module fragment - `module;` and preprocessing
      directives - and comments, so the first line of anything else ends the scan. `module` is not
      a reserved word, so a line beginning with it is a declaration only where what follows is
      spelled as a module name; otherwise it is ordinary code, and likewise ends the scan. The
      declaration is recognized only where [cpp.pre] lets a directive appear: first on its line,
      after whitespace containing no newline.

      The build records what a unit includes and imports, so neither is read here; what it cannot
      record is which module an implementation unit belongs to, since `module M;` and `import M;`
      look alike to a dependency scan, and that is what this supplies.
   */
  [[nodiscard]]
  std::optional<module_declaration> scan_module_declaration(std::istream& source);

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

  /** \brief The tests which must run: those stale since the previous run, and those it left failing.

      Absent when there is no stamp from a previous run to judge staleness against. What a test
      depends on is read from the record the build which produced the executable left of it, so
      this throws where there is no such build, or one whose record is not understood - Ninja's and
      Visual Studio's are.
   */
  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>> tests_to_run(const project_paths& projPaths);

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
