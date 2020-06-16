////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Various definitions for test_family.
 */

#include "TestFamily.hpp"

namespace sequoia::testing
{
  auto test_family::execute(const bool writeFiles, const concurrency_mode mode) -> results 
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};
    
    std::vector<log_summary> summaries{};
    summaries.reserve(m_Tests.size());
    bool diagnosticsToWrite{};

    if(mode < concurrency_mode::test)
    {
      summary_writer writer{};
      for(auto& pTest : m_Tests)
      {
        const auto summary{pTest->execute()};
        summaries.push_back(summary);

        if(!summary.diagnostics_output().empty()) diagnosticsToWrite = true;

        writer.to_file(test_summary_filename(*pTest), summary);
      }
    }
    else
    {
      using data = std::pair<std::string, log_summary>;
      std::vector<std::future<data>> results{};
      results.reserve(m_Tests.size());
      for(auto& pTest : m_Tests)
      {
        results.emplace_back(std::async([&pTest](){
              return std::make_pair(test_summary_filename(*pTest), pTest->execute()); }));
      }
      
      summary_writer writer{};
      for(auto& res : results)
      {
        const auto [filename, summary]{res.get()};
        summaries.push_back(summary);

        if(!summary.diagnostics_output().empty()) diagnosticsToWrite = true;

        writer.to_file(filename, summary);
      }
    }

    if(writeFiles && diagnosticsToWrite)
    {
      if(auto filename{diagnostics_filename()}; !filename.empty())
      {
        if(std::ofstream file{filename.data()})
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

  std::string test_family::diagnostics_filename() const
  {
    std::string name{m_Name};
    for(auto& c : name) if(c == ' ') c = '_';

    // TO DO: use filesystem
    
    return std::string{"../output/DiagnosticsOutput/"}.append(std::move(name)).append("_FPOutput.txt");
  }

  std::string test_family::test_summary_filename(const test& t)
  {
    // TO DO: use filesystem

    std::string name{t.source_file_name()};    
    name.erase(name.find_last_of('.') + 1);
    name.append("txt");
    
    const auto pos{name.find_last_of('/')};
    name = name.substr(pos);

    return std::string{"../output/TestSummaries/"}.append(name);
  }

  void test_family::summary_writer::to_file(std::string_view filename, const log_summary& summary)
  {
    auto mode{std::ios_base::out};
    if(auto found{m_Record.find(filename)}; found != m_Record.end())
    {
      mode = std::ios_base::app;
    }
    else
    {
      m_Record.insert(std::string{filename});
    }

    if(std::ofstream file{filename.data()})
    {
      //file << summary << "\n\n";
    }
    else
    {
      throw std::runtime_error{std::string{"Unable to open summaries file "}.append(filename).append(" for writing\n")};
    }
  }
}
