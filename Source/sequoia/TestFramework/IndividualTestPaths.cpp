////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for IndividualTestPaths.hpp
 */

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

    [[nodiscard]]
    fs::path versioned_diagnostics(fs::path dir, std::string_view suite, const fs::path& source, test_mode mode, std::string_view suffix,const std::optional<std::string>& platform)
    {
      const auto file{
        fs::path{source}.filename()
                        .replace_extension()
                        .concat("_")
                        .concat(to_tag(mode))
                        .concat(suffix)
                        .concat((platform && !platform->empty()) ? "_" + platform.value() : "")
                        .concat(".txt")};

      return (dir /= fs::path{replace_all(suite, " ", "_")}) /= file;
    }

    [[nodiscard]]
    fs::path test_summary_filename(const fs::path& sourceFile, const project_paths& projectPaths, const std::optional<std::string>& discriminator)
    {

      if(sourceFile.empty())
        throw std::runtime_error("Source files should have a non-trivial name!");

      const auto name{
          [&]() {
            auto summaryFile{fs::path{sourceFile}.replace_extension(".txt")};
            if(discriminator && !discriminator->empty())
              summaryFile.replace_filename(summaryFile.stem().concat("_" + discriminator.value()).concat(summaryFile.extension().string()));

            return summaryFile;
          }()
      };

      if(!name.is_absolute())
      {
        if(const auto testRepo{projectPaths.tests().repo()}; !testRepo.empty())
        {
          return projectPaths.output().test_summaries() / back(testRepo) / rebase_from(name, testRepo);
        }
      }
      else
      {
        auto summaryFile{projectPaths.output().test_summaries()};
        auto iters{std::ranges::mismatch(name, summaryFile)};

        while(iters.in1 != name.end())
          summaryFile /= *iters.in1++;

        return summaryFile;
      }

      return name;
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

  individual_diagnostics_paths::individual_diagnostics_paths(const fs::path& projectRoot, std::string_view suite, const fs::path& source, test_mode mode, const std::optional<std::string>& platform)
    : m_Diagnostics{versioned_diagnostics(output_paths::diagnostics(projectRoot), suite, source, mode, "Output", platform)}
    , m_CaughtExceptions{versioned_diagnostics(output_paths::diagnostics(projectRoot), suite, source, mode, "Exceptions", platform)}
  {}

  //===================================== individual_diagnostics_paths =====================================//

  test_summary_path::test_summary_path(const fs::path& sourceFile, const project_paths& projectPaths, const std::optional<std::string>& summaryDiscriminator)
    : m_Summary{test_summary_filename(sourceFile, projectPaths, summaryDiscriminator)}
  {}
}
