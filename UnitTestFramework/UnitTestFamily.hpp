////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

/*! \file UnitTestFamily.hpp
    \brief Utilities for grouping unit tests

 */

namespace sequoia::unit_testing
{
  class test_family
  {
  public:
    template<class... Tests>
    test_family(std::string_view name, Tests&&... tests) : m_Name{name}
    {
      register_tests(std::forward<Tests>(tests)...);
    }

    std::vector<log_summary> execute()
    {
      std::vector<log_summary> summaries{};
      bool write{};
      for(auto& pTest : m_Tests)
      {
        summaries.push_back(pTest->execute());
        if(!summaries.back().versioned_output().empty()) write = true;
      }

      if(write)
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

      return summaries;
    }

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }

  private:
    std::vector<std::unique_ptr<test>> m_Tests{};
    std::string m_Name;

    [[nodiscard]]
    std::string false_positive_filename()
    {
      std::string name{m_Name};
      for(auto& c : name)
      {
        if(c == ' ') c = '_';
      }
    
      return std::string{"../output/DiagnosticsOutput/"}.append(std::move(name)).append("_FPOutput.txt");
    }

    template<class Test, class... Tests>
    void register_tests(Test&& t, Tests&&... tests)
    {
      m_Tests.emplace_back(std::make_unique<Test>(std::forward<Test>(t)));
      register_tests(std::forward<Tests>(tests)...);
    }

    void register_tests() {}
  };
}
