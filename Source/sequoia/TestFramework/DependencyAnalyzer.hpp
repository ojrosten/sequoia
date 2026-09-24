////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Selects the tests to run from what has changed since the previous run.

    A run leaves three things behind:
    -# A stamp, holding the run's time;
    -# The tests to run next time regardless: those which failed, and those the run left out;
    -# The passes of any selection it ran.

    `prune` reads those, and the build's record of what each test was built from, and selects
    the tests which are stale or are to be rerun.

    The same record says whether the library itself has changed since the executable was built, which
    `create` asks before it writes anything from code which may be out of date.
 */

#include "sequoia/TestFramework/CMakeCache.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <chrono>
#include <format>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <variant>

namespace sequoia::testing
{
  /** \brief Whether a run selects its tests by what has changed since the previous run. */
  enum class prune_mode { passive, active };

  /** \brief A test's source, and the time of the run which recorded the test as passing or failing. */
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

  /** \brief The time after which a modification counts as later than the run which wrote `stamp`.

      -# Where the filesystem records sub-second times, the threshold is the stamp itself.
      -# Where the filesystem truncates to whole seconds - libstdc++ does on macOS - a modification
         made during the run can carry a time up to a second before the stamp. There the threshold
         is a second earlier, which resolves the ambiguous cases as stale: a re-run, never a
         missed test.

      A stamp landing exactly on a second cannot be told from a truncated one, and is treated as
      truncated. A filesystem coarser than a second - exFAT, some network mounts - looks the same
      here and remains partly exposed.
   */
  [[nodiscard]]
  std::filesystem::file_time_type staleness_threshold(std::filesystem::file_time_type stamp);

  /** \brief The prune records in `file`; none if there is no such file.

      \throws std::runtime_error if the file does not parse as prune records.
   */
  [[nodiscard]]
  std::vector<prune_record> read_tests(const std::filesystem::path& file);

  /** \brief Writes `tests` to `file`, each source path made relative to the tests repository. */
  void write_tests(const project_paths& projPaths, const std::filesystem::path& file, std::span<const prune_record> tests);

  /** \brief Why a requested prune selects every test. */
  enum class prune_fallback_reason { no_previous_stamp, toolchain_changed };

  [[nodiscard]]
  std::string to_string(prune_fallback_reason reason);

  /** \brief The tests which should run, judged against the previous run's stamp.

      A test is stale if any of the following changed after the stamp:
      -# The TU containing the test;
      -# Any explicit dependency of the test TU, such as a header on which it transitively depends;
      -# Any implicit dependency of the test TU, such as a source implementing the declarations in the
         headers of item 2;
      -# Any materials associated with the test.

      A test recorded as passing since that change is not stale.

      \pre Definitions complementing a declaration are found in either:
      -# One of the headers where the TU sees the declaration;
      -# A source file with the same stem as one of the headers of the previous item.

      \returns One of:
      -# A `prune_fallback_reason`, meaning every test should run, if:
         -# No previous run left a stamp;
         -# A toolchain header changed after the stamp.
      -# Otherwise the stale tests together with those the previous run left failing, sorted, each once.

      \throws std::runtime_error if the build's record of dependencies:
      -# Does not exist, because nothing has been built;
      -# Is in an unknown format - currently Ninja's and MSBuild's are understood;
      -# Is corrupted.
   */
  [[nodiscard]]
  std::variant<std::vector<std::filesystem::path>, prune_fallback_reason> tests_to_run(const project_paths& projPaths);

  /** \brief After a run of every test not left out: records the tests to rerun - the failures and
             those left out - forgets the selected passes, and stamps the run's time.
   */
  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> testsToRerun,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  /** \brief After a run of a selection: records which of the executed tests passed and which failed,
             alongside those already recorded. The stamp is untouched.
   */
  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> executedTests,
                          std::span<const std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  /** \brief The directory of sources this library was compiled from, as the compiler was given it.

      Relative when the compiler was handed a relative path, or when the build remaps the paths the
      binary records - `-fmacro-prefix-map`, which `-ffile-prefix-map` implies - in which case the
      library's own files cannot be told apart.
   */
  [[nodiscard]]
  std::filesystem::path sequoia_library_root();

  /** \brief Refuses to go on if the library beneath `libraryRoot` has changed since the executable was
             built: what the executable would write comes from code older than the library's.

      The library's objects are those compiled from a source beneath `libraryRoot`, and its own files are
      those of the files read to compile them which lie beneath it too. So a header of the library which
      only the tests read does not count, and nor does anything of the toolchain's, of another library's,
      or of the tests'. The build's record is read only if something beneath `libraryRoot` - a file, or a
      directory, whose time a deletion within it moves - is no older than the executable.

      Where the question cannot be answered, a one-line warning to `stream` gives the reason, and nothing
      is refused: `libraryRoot` is relative; the executable cannot be found; the build's record cannot be
      read; or the record names no object compiled from beneath `libraryRoot`.

      \throws std::runtime_error naming the newest of the library's own files, the object it was read to
      compile and that object's target, and the time stamps of the file and the executable, if the file is
      no older than the executable; or naming a file of the library's which the build read and which
      cannot now be read.
   */
  void refuse_if_library_changed_since_build(const project_paths& projPaths,
                                             const cmake_cache& cache,
                                             const std::filesystem::path& libraryRoot,
                                             std::ostream& stream);

  /** \brief Empties the directory in which the repetitions of an instability analysis leave their prune files. */
  void setup_instability_analysis_prune_folder(const project_paths& projPaths);

  /** \brief Folds the repetitions' prune files into the run's, then removes the repetitions' files.

      A test to rerun after any repetition is to rerun; a test passing in every repetition passed.
   */
  void aggregate_instability_analysis_prune_files(const project_paths& projPaths,
                                                  prune_mode mode,
                                                  std::filesystem::file_time_type timeStamp,
                                                  std::size_t numReps);
}

namespace std
{
  template<>
  struct formatter<sequoia::testing::prune_fallback_reason>
  {
    constexpr auto parse(auto& ctx) { return ctx.begin(); }

    auto format(sequoia::testing::prune_fallback_reason reason, auto& ctx) const -> decltype(ctx.out())
    {
      return std::format_to(ctx.out(), "{}", sequoia::testing::to_string(reason));
    }
  };
}
