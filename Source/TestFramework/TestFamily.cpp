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
  auto test_family::execute(const output_mode outputMode, const concurrency_mode concurrenyMode) -> results 
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};
    
    std::vector<log_summary> summaries{};
    summaries.reserve(m_Tests.size());
    bool diagnosticsToWrite{};    
    summary_writer writer{};

    namespace fs = std::filesystem;
    
    auto summarizer{
      [&summaries, &diagnosticsToWrite, &writer](const log_summary& summary, const fs::path& filename){
        summaries.push_back(std::move(summary));

        if(!summary.diagnostics_output().empty()) diagnosticsToWrite = true;

        writer.to_file(std::move(filename), summary);
      }
    };

    process_materials([](const fs::path&, const fs::path& output){
                                fs::remove_all(output);
                      });

    process_materials([](const fs::path& materials, const fs::path& output){
                        fs::create_directories(output);
                        fs::copy(materials, output, fs::copy_options::recursive | fs::copy_options::update_existing);
                      });
    
    if(concurrenyMode < concurrency_mode::test)
    {
      for(auto& pTest : m_Tests)
      {
        const auto summary{pTest->execute()};
        summarizer(summary, test_summary_filename(*pTest, outputMode));
      }
    }
    else
    {
      using data = std::pair<log_summary, std::filesystem::path>;
      std::vector<std::future<data>> results{};
      results.reserve(m_Tests.size());
      for(auto& pTest : m_Tests)
      {
        results.emplace_back(std::async([&test{*pTest}, outputMode](){
              return std::make_pair(test.execute(), test_summary_filename(test, outputMode)); }));
      }
      
      summary_writer writer{};
      for(auto& res : results)
      {
        const auto [summary, filename]{res.get()};
        summarizer(summary, filename);
      }
    }

    if((outputMode == output_mode::write_files) && diagnosticsToWrite)
    {
      if(auto filename{diagnostics_filename()}; !filename.empty())
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
          throw std::runtime_error{std::string{"Unable to open diagnostics output file "}.append(filename).append(" for writing\n")};
        }
      }
    }

    return {steady_clock::now() - time, std::move(summaries)};
  }

  template<class Fn>
    requires invocable<Fn, std::filesystem::path, std::filesystem::path> 
  void test_family::process_materials(Fn fn)
  {
    namespace fs = std::filesystem;
    
    for(auto& pTest : m_Tests)
    {
      const auto& materials{pTest->materials()};
      if(fs::exists(materials))
      {
        const auto rel{fs::relative(materials, test_materials_path())};
        const auto output{output_path("TestsOutput") /= rel};

        fn(materials, output);        
      }
    }
  }
  

  std::filesystem::path test_family::diagnostics_filename() const
  {
    auto make_name{
      [](std::string n){
        for(auto& c : n) if(c == ' ') c = '_';

        return n.append("_FPOutput.txt");
      }
    };

    return output_path("DiagnosticsOutput").append(make_name(m_Name));
  }     

  std::filesystem::path test_family::test_summary_filename(const test& t, const output_mode outputMode)
  {
    namespace fs = std::filesystem;
    
    if(outputMode == output_mode::none) return "";

    const auto name{fs::path{t.source_file_name()}.replace_extension(".txt")};
    if(name.empty())
      throw std::logic_error("Source files should have a non-trivial name!");

    fs::path absolute{output_path("TestSummaries")};
    for(auto i{std::next(name.begin())}; i != name.end(); ++i)
    {
      absolute /= *i;
    }

    return absolute;
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

    if(std::ofstream file{filename, mode})
    {
      file << summarize(summary, log_verbosity::failure_messages, no_indent, no_indent);
    }
    else
    {
      throw std::runtime_error{std::string{"Unable to open summaries file "}.append(filename).append(" for writing\n")};
    }
  }

  [[nodiscard]]
  std::string summarize(const test_family::summary& summary, const log_verbosity suppression, indentation ind_0, indentation ind_1)
  {
    return summarize(summary.log, summary.execution_time, suppression, ind_0, ind_1);
  }
}
