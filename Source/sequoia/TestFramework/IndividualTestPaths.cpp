////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Definitions for IndividualTestPaths.hpp
 */

#include "sequoia/TestFramework/IndividualTestPaths.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    fs::path versioned_diagnostics(std::filesystem::path dir, std::string_view family, std::string_view source, std::string_view mode, std::string_view suffix)
    {
      const auto file{
              fs::path{source}.filename()
                              .replace_extension()
                              .concat("_")
                              .concat(mode)
                              .concat(suffix)
                              .concat(".txt")};

      return (dir /= fs::path{replace_all(family, " ", "_")}) /= file;
    }
  }

  //===================================== individual_materials_paths =====================================//

  individual_materials_paths::individual_materials_paths(fs::path sourceFile, const project_paths& projPaths)
    : individual_materials_paths{rebase_from(sourceFile.replace_extension(), projPaths.tests().repo()), projPaths.test_materials(), projPaths.output()}
  {}

  individual_materials_paths::individual_materials_paths(const fs::path& relativePath, const test_materials_paths& materials, const output_paths& output)
    : m_Materials{materials.repo() / relativePath}
    , m_TemporaryMaterials{output.tests_temporary_data() / relativePath}
  {}

  [[nodiscard]]
  fs::path individual_materials_paths::working() const
  {
    if(m_Materials.empty()) return "";

    return fs::exists(prediction()) ? m_TemporaryMaterials / "WorkingCopy" : m_TemporaryMaterials;
  }

  [[nodiscard]]
  std::filesystem::path individual_materials_paths::original_working() const
  {
    if(m_Materials.empty()) return "";

    return fs::exists(prediction()) ? m_Materials / "WorkingCopy" : m_Materials;
  }

  [[nodiscard]]
  fs::path individual_materials_paths::original_auxiliary() const
  {
    return fs::exists(prediction()) ? m_Materials / "Auxiliary" : "";
  }

  [[nodiscard]]
  fs::path individual_materials_paths::auxiliary() const
  {
    return fs::exists(prediction()) ? m_TemporaryMaterials / "Auxiliary" : "";
  }

  [[nodiscard]]
  fs::path individual_materials_paths::prediction() const
  {
    const auto p{m_Materials / "Prediction"};

    return fs::exists(p) ? p : "";
  }

  //===================================== individual_diagnostics_paths =====================================//

  individual_diagnostics_paths::individual_diagnostics_paths(std::filesystem::path projectRoot, std::string_view family, std::string_view source, std::string_view mode)
    : m_Diagnostics{versioned_diagnostics(output_paths::diagnostics(projectRoot), family, source, mode, "Output")}
    , m_CaughtExceptions{versioned_diagnostics(output_paths::diagnostics(projectRoot), family, source, mode, "Exceptions")}
  {}
}