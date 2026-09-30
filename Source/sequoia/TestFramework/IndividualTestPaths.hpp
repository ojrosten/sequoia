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

  /** \brief Where a test's materials are: fixed on construction, whatever exists on disk.

      A test's materials have two roots. The *original* root, in `TestMaterials`, holds the test's
      materials as written; the *temporary* root, in `output/TestsTemporaryData`, holds what a test
      works with as it runs. Both mirror the path of the test's source file, minus its extension,
      with the name of the test's class as the leaf.

      Beneath the original root, the directories `WorkingCopy`, `Prediction` and `Auxiliary` carry
      special meaning. If present, `WorkingCopy` and `Auxiliary` are reproduced beneath the temporary
      root; predictions are not part of a test's execution context, so `prediction()` is a path
      beneath the original root.

      A test whose original materials vary with the configuration declares a materials
      discriminator. The test's original materials then sit one level down, in a directory within
      the test's own directory. The discriminator names that directory, and only that directory is
      the original root. So a run can neither read nor update the materials of another configuration.

      The discriminator is kept here as it is given. Whether the discriminator names a directory at
      all is judged when the materials are prepared. An empty discriminator, for example, is refused
      then.

      The temporary root has no such level. The temporary root is wiped each time the materials are
      prepared. Within one build tree, it holds the materials of one configuration at a time. Two
      build trees run at once share it (roadmap item 224).

      Every path is returned whether or not anything is there; which of them exist is for the
      caller to ask. A default-constructed instance names no test: its two roots are empty, and
      asking it for any other path throws `std::logic_error`.
   */
  class individual_materials_paths
  {
  public:
    individual_materials_paths() = default;

    individual_materials_paths(const std::filesystem::path& sourceFile,
                               std::string_view testName,
                               const project_paths& projPaths,
                               const std::optional<std::string>& materialsDiscriminator);

    /** \brief The path of the test's own directory in `TestMaterials`.

        If no materials discriminator was given, this directory is the original root. Otherwise the
        discriminator names a directory within this directory, and that directory is the original root.
     */
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

    [[nodiscard]]
    std::filesystem::path original_materials_root() const;

    [[nodiscard]]
    const std::filesystem::path& temporary_materials_root() const noexcept
    {
      return m_TemporaryMaterialsRoot;
    }

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

  /** \brief Where a test records its last execution: when it started and, once it has finished, how long it took.

      A record naming a start and no duration marks a test that was executing when its run ended. The path is empty
      for default project paths.
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
