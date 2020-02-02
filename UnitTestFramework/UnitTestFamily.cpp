////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestFamily.hpp"

namespace sequoia::unit_testing
{
  auto test_family::execute(const bool writeFiles, const bool asynchronous) -> results 
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};
    
    std::vector<log_summary> summaries{};
    summaries.reserve(m_Tests.size());
    bool dataToWrite{};

    if(!asynchronous)
    {
      for(auto& pTest : m_Tests)
      {
        summaries.push_back(pTest->execute());
        if(!summaries.back().versioned_output().empty()) dataToWrite = true;
      }
    }
    else
    {
      std::vector<std::future<log_summary>> results{};
      results.reserve(m_Tests.size());
      for(auto& pTest : m_Tests)
      {
        results.emplace_back(std::async([&pTest](){
              return pTest->execute(); }));
      }

      for(auto& res : results)
      {
        summaries.push_back(res.get());
        if(!summaries.back().versioned_output().empty()) dataToWrite = true;
      } 
    }

    if(writeFiles && dataToWrite)
    {
      if(auto filename{false_positive_filename()}; !filename.empty())
      {
        if(std::ofstream file{filename.data()})
        {           
          for(const auto& s : summaries)
          {           
            file << s.versioned_output().data();
          }
        }
        else
        {
          throw std::runtime_error{std::string{"Unable to open versioned file "}.append(filename).append(" for writing\n")};
        }
      }
    }

    return {steady_clock::now() - time, std::move(summaries)};
  }

  std::string test_family::false_positive_filename()
  {
    std::string name{m_Name};
    for(auto& c : name)
    {
      if(c == ' ') c = '_';
    }
    
    return std::string{"../output/DiagnosticsOutput/"}.append(std::move(name)).append("_FPOutput.txt");
  }
}
