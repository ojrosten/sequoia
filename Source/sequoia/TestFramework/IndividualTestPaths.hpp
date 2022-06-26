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

    [[nodiscard]]
    friend bool operator!=(const individual_materials_paths&, const individual_materials_paths&) noexcept = default;
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

    individual_diagnostics_paths(std::filesystem::path projectRoot, std::string_view family, std::string_view source, std::string_view mode);

    [[nodiscard]]
    const std::filesystem::path& diagnostics_file() const noexcept
    {
      return m_Diagnostics;
    }

    [[nodiscard]]
    const std::filesystem::path& caught_exceptions_file() const noexcept
    {
      return m_CaughtExceptions;
    }

    [[nodiscard]]
    friend bool operator==(const individual_diagnostics_paths&, const individual_diagnostics_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const individual_diagnostics_paths&, const individual_diagnostics_paths&) noexcept = default;
  private:
    std::filesystem::path
      m_Diagnostics,
      m_CaughtExceptions;
  };
}