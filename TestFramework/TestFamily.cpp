////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
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
  auto test_family::execute(const bool writeFiles, const concurrency_mode mode) -> results 
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
    
    if(mode < concurrency_mode::test)
    {
      for(auto& pTest : m_Tests)
      {
        const auto summary{pTest->execute()};
        summarizer(summary, test_summary_filename(*pTest, writeFiles));
      }
    }
    else
    {
      using data = std::pair<log_summary, std::filesystem::path>;
      std::vector<std::future<data>> results{};
      results.reserve(m_Tests.size());
      for(auto& pTest : m_Tests)
      {
        results.emplace_back(std::async([&pTest, writeFiles](){
              return std::make_pair(pTest->execute(), test_summary_filename(*pTest, writeFiles)); }));
      }
      
      summary_writer writer{};
      for(auto& res : results)
      {
        const auto [summary, filename]{res.get()};
        summarizer(summary, filename);
      }
    }

    if(writeFiles && diagnosticsToWrite)
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

  std::filesystem::path test_family::diagnostics_filename() const
  {
    auto make_name{
      [](std::string n){
        for(auto& c : n) if(c == ' ') c = '_';

        return n.append("_FPOutput.txt");
      }
    };

    return get_output_path("DiagnosticsOutput").append(make_name(m_Name));
  }

  std::filesystem::path test_family::get_output_path(std::string_view subDirectory)
  {    
    namespace fs = std::filesystem;
    return fs::current_path().parent_path().append("output").append(subDirectory);
  }
     

  std::filesystem::path test_family::test_summary_filename(const test& t, const bool writeFiles)
  {
    namespace fs = std::filesystem;
    
    if(!writeFiles) return "";

    const auto name{fs::path{t.source_file_name()}.replace_extension(".txt")};
    if(name.empty())
      throw std::logic_error("Source files should have a non-trivial name!");

    fs::path stripped{};
    for(auto i{std::next(name.begin())}; i != name.end(); ++i)
    {
      stripped /= *i;
    }

    return get_output_path("TestSummaries") /= stripped;
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
      file << summarize(summary, log_verbosity::failure_messages, "", "");
    }
    else
    {
      throw std::runtime_error{std::string{"Unable to open summaries file "}.append(filename).append(" for writing\n")};
    }
  }

  [[nodiscard]]
  std::string summarize(const test_family::summary& summary, const log_verbosity suppression, std::string_view indent_0, std::string_view indent_1)
  {
    return summarize(summary.log, summary.execution_time, suppression, indent_0, indent_1);
  }
}
