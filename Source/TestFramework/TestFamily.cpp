////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestFamily.hpp.
 */

#include "TestFamily.hpp"
#include "Summary.hpp"

namespace sequoia::testing
{
  void test_family::set_materials(test& t)
  {
    namespace fs = std::filesystem;

    t.test_repository(m_TestRepo);

    const auto rel{
      [&t, &testRepo{m_TestRepo}, &materialsRepo{m_TestMaterialsRepo}](){
        if(testRepo.empty()) return fs::path{};

        auto folderName{fs::path{t.source_filename()}.replace_extension()};
        if(folderName.is_absolute())
          folderName = fs::relative(folderName, materialsRepo);

        return rebase_from(folderName, testRepo);
      }()
    };

    const auto materials{!rel.empty() ? m_TestMaterialsRepo / rel : fs::path{}};
    if(fs::exists(materials))
    {
      const auto output{tests_temporary_data_path(m_OutputDir) /= rel};

      const auto[original, workingCopy, prediction]{
         [&output,&materials] () -> std::tuple<fs::path, fs::path, fs::path>{
          const auto prediction{materials / "Prediction"};
          const auto original{materials / "WorkingCopy"};

          if(fs::exists(original) && fs::exists(prediction))
          {
            return {original, output / "WorkingCopy", prediction};
          }

          return {materials, output, ""};
        }()
      };

      t.materials(workingCopy, prediction);

      if(std::find(m_MaterialsPaths.cbegin(), m_MaterialsPaths.cend(), workingCopy) == m_MaterialsPaths.cend())
      {
        fs::remove_all(output);
        fs::create_directories(output);
        fs::copy(original, workingCopy, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        m_MaterialsPaths.emplace_back(workingCopy);
      }
    }
  }

  auto test_family::execute(const output_mode outputMode, const concurrency_mode concurrenyMode) -> results 
  {
    using namespace std::chrono;
    namespace fs = std::filesystem;

    const auto time{steady_clock::now()};

    std::vector<log_summary> summaries{};
    summaries.reserve(m_Tests.size());
    bool hasDiagnostics{};
    summary_writer writer{};

    auto compare{
      [](const paths& lhs, const paths& rhs){
        return lhs.workingMaterials < rhs.workingMaterials;
      }
    };

    std::set<paths, decltype(compare)> updateables{};

    auto process{
      [&summaries, &hasDiagnostics, &writer, &updateables](log_summary summary, const paths& files){
        summaries.push_back(std::move(summary));

        if(!summaries.back().diagnostics_output().empty()) hasDiagnostics = true;

        writer.to_file(files.summary, summaries.back());

        if((files.mode & output_mode::update_materials) == output_mode::update_materials)
        {
          if(summary.soft_failures())
          {
            if(fs::exists(files.workingMaterials) && fs::exists(files.predictions))
            {
              updateables.insert(files);
            }
          }
        }
      }
    };

    if(concurrenyMode < concurrency_mode::test)
    {
      for(auto& pTest : m_Tests)
      {
        const auto summary{pTest->execute()};
        process(summary, paths{*pTest, outputMode, m_OutputDir, m_TestRepo});
      }
    }
    else
    {
      using data = std::pair<log_summary, paths>;
      std::vector<std::future<data>> results{};
      results.reserve(m_Tests.size());
      for(auto& pTest : m_Tests)
      {
        results.emplace_back(
          std::async([&test{*pTest}, outputMode, outputDir{m_OutputDir}, testRepo{m_TestRepo}](){
            return std::make_pair(test.execute(), paths{test, outputMode, outputDir, testRepo}); })
        );
      }

      for(auto& res : results)
      {
        const auto [summary, paths]{res.get()};
        process(summary, paths);
      }
    }

    if((outputMode & output_mode::write_files) == output_mode::write_files)
    {
      if(auto filename{diagnostics_filename()}; !filename.empty())
      {
        if(hasDiagnostics || fs::exists(filename))
        {
          if(std::ofstream file{filename})
          {
            for(const auto& s : summaries)
            {
              file << s.diagnostics_output().data();
            }
          }
          else
          {
            throw std::runtime_error{report_failed_write(filename)};
          }
        }
      }
    }

    for(const auto& update : updateables)
    {
      fs::remove_all(update.predictions);
      
      fs::copy(update.workingMaterials, update.predictions, fs::copy_options::recursive);
    }

    return {steady_clock::now() - time, std::move(summaries)};
  }

  std::filesystem::path test_family::diagnostics_filename() const
  {
    auto make_name{
      [](std::string n){
        for(auto& c : n) if(c == ' ') c = '_';

        return n.append("_FPOutput.txt");
      }
    };

    return diagnostics_output_path(m_OutputDir) /= make_name(m_Name);
  }

  std::filesystem::path test_family::test_summary_filename(const test& t, const output_mode outputMode, const std::filesystem::path& outputDir, const std::filesystem::path& testRepo)
  {
    namespace fs = std::filesystem;

    if((outputMode & output_mode::write_files) != output_mode::write_files) return "";

    const auto name{t.source_filename().replace_extension(".txt")};
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

  void test_family::summary_writer::to_file(const std::filesystem::path& filename, const log_summary& summary)
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

  test_family::paths::paths(const test& t, output_mode outputMode, const std::filesystem::path& outputDir, const std::filesystem::path& testRepo)
    : mode{outputMode}
    , summary{test_summary_filename(t, outputMode, outputDir, testRepo)}
    , workingMaterials{t.working_materials()}
    , predictions{t.predictive_materials()}
  {}

  [[nodiscard]]
  std::string summarize(const test_family::summary& summary, const summary_detail suppression, indentation ind_0, indentation ind_1)
  {
    return summarize(summary.log, summary.execution_time, suppression, ind_0, ind_1);
  }
}
