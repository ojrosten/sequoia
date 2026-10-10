////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief File paths pertaining to individual tests.
 */

#include "sequoia/TestFramework/ProjectPaths.hpp"
#include "sequoia/TestFramework/TestMode.hpp"

namespace sequoia::testing
{
  /** \brief The path of `sourceFile` relative to the tests' repository, less its extension.

      Each test in `sourceFile` keys its materials on this path, with its own name as the leaf.
   */
  [[nodiscard]]
  std::filesystem::path materials_prefix(const std::filesystem::path& sourceFile, const project_paths& projPaths);

  inline constexpr std::optional<std::string> null_discriminator{};

  /** \brief Paths for the test's materials.

      The test's materials have three roots:
      -# The original root, `original_materials_root()`, is in
         `TestMaterials`, and holds the test's materials as written;
      -# The temporary root, `temporary_materials_root()`, is in
         `output/TestsTemporaryData`, and holds what the test works with as it
         runs;
      -# The discarded root, `discarded_materials_root()`, is the temporary
         root's sibling, and holds the temporary root of the test's previous
         run until the runner removes it.

      The original and temporary roots mirror the path of the test's source
      file, less its extension, with the name of the test's class as the leaf.
      If a materials discriminator was given, the original root is one level
      further down, as `original_materials_root()` describes. The temporary
      root has no such level.

      `original_working()`, `prediction()` and `original_auxiliary()` are the paths of `WorkingCopy`,
      `Prediction` and `Auxiliary` under the original root. `working()` and `auxiliary()` are the
      paths of `WorkingCopy` and `Auxiliary` under the temporary root. `Prediction` has no temporary
      counterpart, since the predictions are not part of the test's execution context.

      A default-constructed instance names no test, and its original and
      temporary roots are empty.

      \throws std::logic_error if a default-constructed instance is asked for
      the discarded root, or for the path of `WorkingCopy`, `Prediction` or
      `Auxiliary`.
   */
  class individual_materials_paths
  {
  public:
    individual_materials_paths() = default;

    individual_materials_paths(const std::filesystem::path& sourceFile,
                               std::string_view testName,
                               const project_paths& projPaths,
                               const std::optional<std::string>& materialsDiscriminator);

    [[nodiscard]]
    const std::filesystem::path& original_test_root() const noexcept
    {
      return m_OriginalTestRoot;
    }

    [[nodiscard]]
    const std::optional<std::string>& materials_discriminator() const noexcept
    {
      return m_MaterialsDiscriminator;
    }

    /** \brief The original root: `original_test_root()`, followed by the materials discriminator if one
        was given.
     */
    [[nodiscard]]
    std::filesystem::path original_materials_root() const;

    [[nodiscard]]
    const std::filesystem::path& temporary_materials_root() const noexcept
    {
      return m_TemporaryMaterialsRoot;
    }

    /** \brief The temporary root, followed by `.discarded`. */
    [[nodiscard]]
    std::filesystem::path discarded_materials_root() const;

    [[nodiscard]]
    std::filesystem::path original_working() const;

    [[nodiscard]]
    std::filesystem::path working() const;

    [[nodiscard]]
    std::filesystem::path prediction() const;

    [[nodiscard]]
    std::filesystem::path original_auxiliary() const;

    [[nodiscard]]
    std::filesystem::path auxiliary() const;

    [[nodiscard]]
    friend bool operator==(const individual_materials_paths&, const individual_materials_paths&) noexcept = default;
  private:
    std::filesystem::path
      m_OriginalTestRoot{},
      m_TemporaryMaterialsRoot{};

    std::optional<std::string> m_MaterialsDiscriminator{};

    individual_materials_paths(const std::filesystem::path& relativePath,
                               const test_materials_paths& materials,
                               const output_paths& output,
                               const std::optional<std::string>& materialsDiscriminator);
  };

  class individual_diagnostics_paths
  {
  public:
    individual_diagnostics_paths() = default;

    individual_diagnostics_paths(const project_paths& projPaths, std::string_view testName, const std::filesystem::path& source, test_mode mode, const std::optional<std::string>& platform);

    [[nodiscard]]
    const std::filesystem::path& false_positive_or_negative_file_path() const noexcept
    {
      return m_Diagnostics;
    }

    [[nodiscard]]
    const std::filesystem::path& caught_exceptions_file_path() const noexcept
    {
      return m_CaughtExceptions;
    }

    [[nodiscard]]
    friend bool operator==(const individual_diagnostics_paths&, const individual_diagnostics_paths&) noexcept = default;
  private:
    std::filesystem::path
      m_Diagnostics,
      m_CaughtExceptions;
  };

  class test_summary_path {
  public:
    test_summary_path() = default;

    test_summary_path(const std::filesystem::path& sourceFile, std::string_view testName, const project_paths& projectPaths, const std::optional<std::string>& summaryDiscriminator);

    [[nodiscard]]
    const std::filesystem::path& file_path() const noexcept { return m_Summary; }

    [[nodiscard]]
    friend bool operator==(const test_summary_path&, const test_summary_path&) noexcept = default;
  private:
    std::filesystem::path m_Summary;
  };

  /** \brief Where a test records its last execution: when it started and,
             once it has finished, its execution duration and the runner's
             overhead.

      A finished record also gives the attempt which decided each of the
      test's performance checks, one line per check, in the order of the
      checks. A check which threw before its decision has no line.

      A test's record is the last one the runner managed to write. That is
      normally the current run's: the start while the test executes, then the
      whole record once the test has finished. If a write fails, an earlier
      run's record remains. A record naming a start and no execution duration
      marks a test which did not finish in the run that wrote the record, or
      whose finished record could not be written.

      The path is empty for default project paths.
   */
  class test_execution_record_path
  {
  public:
    test_execution_record_path() = default;

    test_execution_record_path(const std::filesystem::path& sourceFile,
                               std::string_view testName,
                               const project_paths& projectPaths);

    [[nodiscard]]
    const std::filesystem::path& file_path() const noexcept { return m_Record; }

    [[nodiscard]]
    friend bool operator==(const test_execution_record_path&, const test_execution_record_path&) noexcept = default;
  private:
    std::filesystem::path m_Record{};
  };
}
