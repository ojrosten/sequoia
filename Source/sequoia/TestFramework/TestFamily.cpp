////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestFamily.hpp
 */

#include "sequoia/TestFramework/TestFamily.hpp"
#include "sequoia/TestFramework/MaterialsUpdater.hpp"
#include "sequoia/TestFramework/Summary.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/Parsing/CommandLineArguments.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::filesystem::path test_summary_filename(const fs::path& sourceFile, const project_paths& projPaths)
    {
      const auto name{fs::path{sourceFile}.replace_extension(".txt")};
      if(name.empty())
        throw std::logic_error("Source files should have a non-trivial name!");
      
      if(!name.is_absolute())
      {
        if(const auto testRepo{projPaths.tests().repo()}; !testRepo.empty())
        {
          return projPaths.output().test_summaries() / back(testRepo) / rebase_from(name, testRepo);
        }
      }
      else
      {
        auto summaryFile{projPaths.output().test_summaries()};
        auto iters{std::mismatch(name.begin(), name.end(), summaryFile.begin(), summaryFile.end())};

        while(iters.first != name.end())
          summaryFile /= *iters.first++;

        return summaryFile;
      }

      return name;
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
    case concurrency_mode::unit:
      return "Unit";
    }

    throw std::logic_error{"Unknown option for concurrency_mode"};
  }


  [[nodiscard]]
  active_recovery_files make_active_recovery_paths(recovery_mode mode, const project_paths& projPaths)
  {
    active_recovery_files paths{};
    if((mode & recovery_mode::recovery) == recovery_mode::recovery)
      paths.recovery_file = projPaths.output().recovery().recovery_file();

    if((mode & recovery_mode::dump) == recovery_mode::dump)
      paths.dump_file = projPaths.output().recovery().dump_file();

    return paths;
  }

  //============================== test_paths ==============================//
  
  test_paths::test_paths(const fs::path& sourceFile,
                         const fs::path& workingMaterials,
                         const fs::path& predictiveMaterials,
                         const project_paths& projPaths)
    : test_file{rebase_from(sourceFile, projPaths.tests().repo())}
    , summary{test_summary_filename(sourceFile, projPaths)}
    , workingMaterials{workingMaterials}
    , predictions{predictiveMaterials}
  {}

  family_processor::family_processor(update_mode mode)
    : m_Mode{mode}
  {}

  //============================== family_processor ==============================//
  
  void family_processor::process(log_summary summary, const test_paths& files)
  {
    if(summary.soft_failures() || summary.critical_failures())
      m_Results.failed_tests.push_back(files.test_file);

    to_file(files.summary, summary);
    
    if(m_Mode != update_mode::none)
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
    if(auto found{m_FilesWrittenTo.find(filename)}; found != m_FilesWrittenTo.end())
    {
      mode = std::ios_base::app;
    }
    else
    {
      m_FilesWrittenTo.insert(filename);
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
  family_results family_processor::finalize_and_acquire()
  {
    for(const auto& update : m_Updateables)
    {
      soft_update(update.workingMaterials, update.predictions);
    }

    m_Results.execution_time = m_Timer.time_elapsed();
    return std::move(m_Results);
  }

  //============================== family_info ==============================//

  [[nodiscard]]
  materials_info family_info::set_materials(const std::filesystem::path& sourceFile, std::vector<std::filesystem::path>& materialsPaths)
  {
    const auto& projPaths{*m_Paths};

    const auto rel{
      [&sourceFile, &projPaths] (){
        if(projPaths.tests().repo().empty()) return fs::path{};

        auto folderName{fs::path{sourceFile}.replace_extension()};

        return rebase_from(folderName, projPaths.tests().repo());
      }()
    };

    const auto materialsDir{!rel.empty() ? m_Paths->test_materials().repo() / rel : fs::path{}};
    if(fs::exists(materialsDir))
    {
      const auto tempOutputDir{projPaths.output().tests_temporary_data() / rel};

      const auto[originalWorkingMaterials, workingCopy, prediction, originalAux, workingAux]{
         [&tempOutputDir,&materialsDir] () -> std::array<fs::path, 5>{
          const auto prediction{materialsDir / test_materials_paths::predictions_folder_name()};

          if(fs::exists(prediction))
          {
            const auto originalWorkingMaterials{materialsDir / test_materials_paths::working_folder_name()};
            const auto workingCopy{tempOutputDir / test_materials_paths::working_folder_name()};

            const auto originalAuxiliaryMaterials{materialsDir / test_materials_paths::auxiliary_folder_name()};
            const auto workingAuxiliaryMaterias{tempOutputDir / test_materials_paths::auxiliary_folder_name()};

            return {originalWorkingMaterials, workingCopy, prediction, originalAuxiliaryMaterials, workingAuxiliaryMaterias};
          }

          return {materialsDir, tempOutputDir};
        }()
      };

      if(std::find(materialsPaths.cbegin(), materialsPaths.cend(), workingCopy) == materialsPaths.cend())
      {
        fs::remove_all(tempOutputDir);
        fs::create_directories(tempOutputDir);
        if(fs::exists(originalWorkingMaterials))
        {
          fs::copy(originalWorkingMaterials, workingCopy, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }
        else
        {
          fs::create_directory(workingCopy);
        }

        if(fs::exists(originalAux))
          fs::copy(originalAux, workingAux, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        materialsPaths.emplace_back(workingCopy);
      }

      return {workingCopy, prediction, workingAux};
    }

    return {};
  }

  family_info::family_info(std::string_view name, const project_paths& projPaths, recovery_mode recoveryMode)
    : m_Name{name}
    , m_Paths{&projPaths}
    , m_Recovery{recoveryMode}
  {}

  [[nodiscard]]
  std::string summarize(const family_summary& summary, const summary_detail suppression, indentation ind_0, indentation ind_1)
  {
    return summarize(summary.log, summary.execution_time, suppression, ind_0, ind_1);
  }
}
