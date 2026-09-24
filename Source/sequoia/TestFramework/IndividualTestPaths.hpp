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
  /** \brief Where a test's materials are: fixed on construction, whatever exists on disk.

      A test's materials have two roots. The *original* root, in `TestMaterials`, holds what is
      committed; the *temporary* root, in `output/TestsTemporaryData`, holds what each run stages
      from it. Both mirror the path of the test's source file, minus its extension, with the name
      of the test's class as the leaf.

      Beneath the original root are `WorkingCopy`, `Prediction` and `Auxiliary`, any of which a
      test may lack. `WorkingCopy` and `Auxiliary` are staged beneath the temporary root under the
      same names; predictions are never staged, so `prediction()` is a path beneath the original
      root.

      Every path is returned whether or not anything is there; which of them exist is for the
      caller to ask. A default-constructed instance names no test, and every path it returns is
      empty.
   */
  class individual_materials_paths
  {
  public:
    individual_materials_paths() = default;

    individual_materials_paths(const std::filesystem::path& sourceFile, std::string_view testName, const project_paths& projPaths);

    [[nodiscard]]
    const std::filesystem::path& original_materials_root() const noexcept
    {
      return m_OriginalMaterialsRoot;
    }

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
      m_OriginalMaterialsRoot,
      m_TemporaryMaterialsRoot;

    individual_materials_paths(const std::filesystem::path& relativePath, const test_materials_paths& materials, const output_paths& output);
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
}