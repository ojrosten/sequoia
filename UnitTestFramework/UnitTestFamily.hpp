////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

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
      for(auto& pTest : m_Tests)
      {
        summaries.push_back(pTest->execute());
      }

      return summaries;
    }

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }      
  private:
    std::vector<std::unique_ptr<test>> m_Tests{};
    std::string m_Name;

    template<class Test, class... Tests>
    void register_tests(Test&& t, Tests&&... tests)
    {
      m_Tests.emplace_back(std::make_unique<Test>(std::forward<Test>(t)));
      register_tests(std::forward<Tests>(tests)...);
    }

    void register_tests() {}
  };
}
