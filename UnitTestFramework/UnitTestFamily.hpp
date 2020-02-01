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
    struct results
    {
      results(log_summary::duration t, std::vector<log_summary> l)
        : execution_time{t}, logs{std::move(l)}
      {}
      
      log_summary::duration execution_time{};
      std::vector<log_summary> logs;
    };

    struct summary
    {
      explicit summary(log_summary::duration t) : execution_time{t}
      {}
      
      log_summary::duration execution_time{};
      log_summary log;
    };
    
    template<class... Tests>
    explicit test_family(std::string_view name, Tests&&... tests) : m_Name{name}
    {
      register_tests(std::forward<Tests>(tests)...);
    }

    [[nodiscard]]
    auto execute(const bool writeFiles, const bool asynchronous) -> results ;

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }

  private:
    std::vector<std::unique_ptr<test>> m_Tests{};
    std::string m_Name;

    [[nodiscard]]
    std::string false_positive_filename();

    template<class Test, class... Tests>
    void register_tests(Test&& t, Tests&&... tests)
    {
      m_Tests.emplace_back(std::make_unique<Test>(std::forward<Test>(t)));
      if constexpr (sizeof...(Tests) > 0)
        register_tests(std::forward<Tests>(tests)...);
    }
  };
}
