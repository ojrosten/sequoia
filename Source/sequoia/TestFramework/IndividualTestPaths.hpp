////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief File paths pertaining to individual tests.
 */

#include "sequoia/TestFramework/ProjectPaths.hpp"
#include "sequoia/TestFramework/TestMode.hpp"

namespace sequoia::testing
{
  class individual_materials_paths
  {
  public:
    individual_materials_paths() = default;

    individual_materials_paths(std::filesystem::path sourceFile, const project_paths& projPaths);

    [[nodiscard]]
    std::filesystem::path original_working() const;

    [[nodiscard]]
    std::filesystem::path working() const;
    
    [[nodiscard]]
    std::filesystem::path original_auxiliary() const;

    [[nodiscard]]
    std::filesystem::path auxiliary() const;

    [[nodiscard]]
    std::filesystem::path prediction() const;

    [[nodiscard]]
    const std::filesystem::path& original_materials() const noexcept
    {
      return m_Materials;
    }

    [[nodiscard]]
    const std::filesystem::path& temporary_materials() const noexcept
    {
      return m_TemporaryMaterials;
    }

    [[nodiscard]]
    friend bool operator==(const individual_materials_paths&, const individual_materials_paths&) noexcept = default;
  private:
    std::filesystem::path
      m_Materials,
      m_TemporaryMaterials;

    individual_materials_paths(const std::filesystem::path& relativePath, const test_materials_paths& materials, const output_paths& output);
  };

  class individual_diagnostics_paths
  {
  public:
    individual_diagnostics_paths() = default;

    individual_diagnostics_paths(const std::filesystem::path& projectRoot, std::string_view suite, const std::filesystem::path& source, test_mode mode, const std::optional<std::string>& platform);

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

    test_summary_path(const std::filesystem::path& sourceFile, const project_paths& projectPaths, const std::optional<std::string>& summaryDiscriminator);

    [[nodiscard]]
    const std::filesystem::path& file_path() const noexcept { return m_Summary; }

    [[nodiscard]]
    friend bool operator==(const test_summary_path&, const test_summary_path&) noexcept = default;
  private:
    std::filesystem::path m_Summary;
  };
}