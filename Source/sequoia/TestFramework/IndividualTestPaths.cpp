////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/IndividualTestPaths.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::string to_tag(test_mode mode)
    {
      switch(mode)
      {
      case test_mode::false_positive:
        return "FP";
      case test_mode::false_negative:
        return "FN";
      case test_mode::standard:
        return "";
      }

      throw std::logic_error{"Unrecognized case for test_mode"};
    }

    /** \brief The directory a test's versioned output belongs in: the mirror of its source
               file's directory, beneath the relevant output root.

        The source file names the directory but not the leaf, because a source file may hold more
        than one test - the false-positive/false-negative pairs do - and keying the leaf on the
        source would have them contend for it.
     */

    [[nodiscard]]
    fs::path test_output_directory(const fs::path& sourceFile, const fs::path& outputRoot, const project_paths& projectPaths)
    {
      if(sourceFile.empty())
        throw std::runtime_error{"Source files should have a non-trivial name!"};

      if(!sourceFile.is_absolute())
      {
        if(const auto testRepo{projectPaths.tests().repo()}; !testRepo.empty())
          return (outputRoot / back(testRepo) / rebase_from(sourceFile, testRepo)).parent_path();

        return sourceFile.parent_path();
      }

      auto dir{outputRoot};
      auto iters{std::ranges::mismatch(sourceFile, dir)};

      while(iters.in1 != sourceFile.end())
        dir /= *iters.in1++;

      return dir.parent_path();
    }

    [[nodiscard]]
    fs::path versioned_diagnostics(const fs::path& source, std::string_view testName, const project_paths& projectPaths, test_mode mode, std::string_view suffix, const std::optional<std::string>& platform)
    {
      const auto file{
        fs::path{testName}.concat("_")
                          .concat(to_tag(mode))
                          .concat(suffix)
                          .concat((platform && !platform->empty()) ? "_" + platform.value() : "")
                          .concat(".txt")};

      return test_output_directory(source, output_paths::diagnostics(projectPaths.project_root()), projectPaths) /= file;
    }

    [[nodiscard]]
    fs::path test_summary_filename(const fs::path& sourceFile, std::string_view testName, const project_paths& projectPaths, const std::optional<std::string>& discriminator)
    {
      const auto file{
        fs::path{testName}.concat((discriminator && !discriminator->empty()) ? "_" + discriminator.value() : "")
                          .concat(".txt")};

      return test_output_directory(sourceFile, projectPaths.output().test_summaries(), projectPaths) /= file;
    }
  }

  //===================================== individual_materials_paths =====================================//

  individual_materials_paths::individual_materials_paths(const fs::path& sourceFile, std::string_view testName, const project_paths& projPaths)
    : individual_materials_paths{rebase_from(sourceFile, projPaths.tests().repo()).parent_path() /= testName, projPaths.test_materials(), projPaths.output()}
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
  fs::path individual_materials_paths::original_working() const
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

  individual_diagnostics_paths::individual_diagnostics_paths(const project_paths& projPaths, std::string_view testName, const fs::path& source, test_mode mode, const std::optional<std::string>& platform)
    : m_Diagnostics{versioned_diagnostics(source, testName, projPaths, mode, "Output", platform)}
    , m_CaughtExceptions{versioned_diagnostics(source, testName, projPaths, mode, "Exceptions", platform)}
  {}

  //===================================== individual_diagnostics_paths =====================================//

  test_summary_path::test_summary_path(const fs::path& sourceFile, std::string_view testName, const project_paths& projectPaths, const std::optional<std::string>& summaryDiscriminator)
    : m_Summary{test_summary_filename(sourceFile, testName, projectPaths, summaryDiscriminator)}
  {}
}
