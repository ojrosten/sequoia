////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestFamily.hpp.
 */

#include "sequoia/TestFramework/TestFamily.hpp"
#include "sequoia/TestFramework/MaterialsUpdater.hpp"
#include "sequoia/TestFramework/Summary.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::filesystem::path test_summary_filename(const fs::path& sourceFile, const std::filesystem::path& outputDir, const std::filesystem::path& testRepo)
    {
      const auto name{fs::path{sourceFile}.replace_extension(".txt")};
      if(name.empty())
        throw std::logic_error("Source files should have a non-trivial name!");

      if(!name.is_absolute())
      {
        if(!testRepo.empty())
        {
          auto back{*(--testRepo.end())};
          return test_summaries_path(outputDir) / back / rebase_from(name, testRepo);
        }
      }
      else
      {
        auto summary{test_summaries_path(outputDir)};
        auto iters{std::mismatch(name.begin(), name.end(), summary.begin(), summary.end())};

        while(iters.first != name.end())
          summary /= *iters.first++;

        return summary;
      }

      return name;
    }
  }

  paths::paths(const fs::path& sourceFile,
               const fs::path& workingMaterials,
               const fs::path& predictiveMaterials,
               update_mode updateMode,
               const std::filesystem::path& outputDir,
               const std::filesystem::path& testRepo)
    : mode{updateMode}
    , test_file{sourceFile}
    , summary{test_summary_filename(sourceFile, outputDir, testRepo)}
    , workingMaterials{workingMaterials}
    , predictions{predictiveMaterials}
  {}

  [[nodiscard]]
  family_results family_processor::finalize_and_acquire()
  {
    for(const auto& update : m_Updateables)
    {
      soft_update(update.workingMaterials, update.predictions);
    }

    m_Results.execution_time = m_Timer.time_elapsed();
    return std::move(m_Results);
  }


  void family_processor::process(log_summary summary, const paths& files)
  {
    if(summary.soft_failures() || summary.critical_failures())
      m_Results.failed_tests.push_back(files.test_file);

    to_file(files.summary, summary);

    if(files.mode != update_mode::none)
    {
      if(summary.soft_failures())
      {
        if(fs::exists(files.workingMaterials) && fs::exists(files.predictions))
        {
          m_Updateables.insert(files);
        }
      }
    }

    m_Results.logs.push_back(std::move(summary));
  }

  void family_processor::to_file(const std::filesystem::path& filename, const log_summary& summary)
  {
    if(filename.empty()) return;

    auto mode{std::ios_base::out};
    if(auto found{m_Record.find(filename)}; found != m_Record.end())
    {
      mode = std::ios_base::app;
    }
    else
    {
      m_Record.insert(filename);
    }

    std::filesystem::create_directories(filename.parent_path());

    if(std::ofstream file{filename, mode})
    {
      file << summarize(summary, summary_detail::failure_messages, no_indent, no_indent);
    }
    else
    {
      throw std::runtime_error{report_failed_write(filename)};
    }
  }

  [[nodiscard]]
  std::string to_string(concurrency_mode mode)
  {
    switch(mode)
    {
    case concurrency_mode::serial:
      return "Serial";
    case concurrency_mode::dynamic:
      return "Dynamic";
    case concurrency_mode::family:
      return "Family";
    case concurrency_mode::test:
      return "Test";
    }

    throw std::logic_error{"Unknown option for concurrency_mode"};
  }

  family_info::materials_setter::materials_setter(family_info& info)
    : m_pInfo{&info}
  {}

  [[nodiscard]]
  materials_info family_info::materials_setter::set_materials(const std::filesystem::path& sourceFile)
  {
    const auto rel{
      [&sourceFile, &testRepo=m_pInfo->m_TestRepo, &materialsRepo=m_pInfo->m_TestMaterialsRepo](){
        if(testRepo.empty()) return fs::path{};

        auto folderName{fs::path{sourceFile}.replace_extension()};
        if(folderName.is_absolute())
          folderName = fs::relative(folderName, materialsRepo);

        return rebase_from(folderName, testRepo);
      }()
    };

    const auto materials{!rel.empty() ? m_pInfo->m_TestMaterialsRepo / rel : fs::path{}};
    if(fs::exists(materials))
    {
      const auto output{tests_temporary_data_path(m_pInfo->m_OutputDir) /= rel};

      const auto[original, workingCopy, prediction, originalAux, workingAux]{
         [&output,&materials] () -> std::array<fs::path, 5>{
          const auto original{materials / "WorkingCopy"};
          const auto prediction{materials / "Prediction"};

          if(fs::exists(prediction))
          {
            const auto auxiliary{materials / "Auxiliary"};
            const auto origAux{fs::exists(auxiliary) ? auxiliary : ""};
            const auto workAux{fs::exists(auxiliary) ? output / "Auxiliary" : ""};
            return {original, output / "WorkingCopy", prediction, origAux, workAux};
          }

          return { materials, output};
        }()
      };

      if(std::find(m_MaterialsPaths.cbegin(), m_MaterialsPaths.cend(), workingCopy) == m_MaterialsPaths.cend())
      {
        fs::remove_all(output);
        fs::create_directories(output);
        if(fs::exists(original))
        {
          fs::copy(original, workingCopy, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }
        else
        {
          fs::create_directory(workingCopy);
        }

        if(fs::exists(originalAux))
          fs::copy(originalAux, workingAux, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        m_MaterialsPaths.emplace_back(workingCopy);
      }

      return {workingCopy, prediction, workingAux};
    }

    return {};
  }

  family_info::family_info(std::string_view name,
                std::filesystem::path testRepo,
                std::filesystem::path testMaterialsRepo,
                std::filesystem::path outputDir,
                recovery_paths recovery)
    : m_Name{name}
    , m_TestRepo{std::move(testRepo)}
    , m_TestMaterialsRepo{std::move(testMaterialsRepo)}
    , m_OutputDir{std::move(outputDir)}
    , m_Recovery{std::move(recovery)}
  {}

  [[nodiscard]]
  std::string summarize(const family_summary& summary, const summary_detail suppression, indentation ind_0, indentation ind_1)
  {
    return summarize(summary.log, summary.execution_time, suppression, ind_0, ind_1);
  }
}
