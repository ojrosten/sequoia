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

namespace sequoia::testing
{
  namespace fs = std::filesystem;

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
}