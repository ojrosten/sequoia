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
  /** \brief Whether the run selects its tests by what has changed since the last. */
  enum class prune_mode { passive, active };

  /** \brief A test, and the time of the run which recorded it as passing or failing. */
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

  /** \brief Reads the prune records in `file`; none if there is no such file.

      Throws `std::runtime_error` if the records cannot be read.
   */
  [[nodiscard]]
  std::vector<prune_record> read_tests(const std::filesystem::path& file);

  /** \brief Writes prune records to `file`, each test's path relative to the tests repository. */
  void write_tests(const project_paths& projPaths, const std::filesystem::path& file, std::span<const prune_record> tests);

  /** \brief The tests which must run: those stale since the previous run, and those it left failing.

      Absent when there is no stamp from a previous run to judge staleness against. What a test
      depends on is read from the record the build which produced the executable left of it, so
      this throws where there is no such build, or one whose record is not understood - Ninja's and
      Visual Studio's are.
   */
  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>> tests_to_run(const project_paths& projPaths);

  /** \brief After a run of every test: records its failures, forgets the selected passes, and stamps the run's time. */
  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  /** \brief After a run of a selection: records which of the tests passed and which failed, beside those already recorded, without moving the stamp. */
  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> executedTests,
                          std::span<const std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  /** \brief Empties the directory in which the repetitions of an instability analysis leave their prune files. */
  void setup_instability_analysis_prune_folder(const project_paths& projPaths);

  /** \brief Folds the repetitions' prune files into the run's - failures are their union, passes their intersection - and removes them. */
  void aggregate_instability_analysis_prune_files(const project_paths& projPaths, prune_mode mode, std::filesystem::file_time_type timeStamp, std::size_t numReps);
}
