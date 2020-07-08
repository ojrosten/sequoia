////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for grouping tests into families.

 */

#include "FreeTestCore.hpp"

#include <vector>
#include <future>
#include <set>
#include <filesystem>

namespace sequoia::testing
{
  /*! \brief Allows tests to be grouped together into a family of related tests

      When tests are executed, it is possible to specify both the concurrency mode
      and whether or not output files (which should generally be version controlled)
      are generated.
   */
  class test_family
  {
  public:
    struct results
    {
      results(log_summary::duration t, std::vector<log_summary> l)
        : execution_time{t}, logs{std::move(l)}
      {}
      
      log_summary::duration execution_time{};
      std::vector<log_summary> logs{};
    };

    struct summary
    {
      explicit summary(log_summary::duration t) : execution_time{t}
      {}
      
      log_summary::duration execution_time{};
      log_summary log{};
    };
    
    template<concrete_test... Tests>
    explicit test_family(std::string_view name, Tests&&... tests) : m_Name{name}
    {
      if constexpr(sizeof...(Tests) > 0)
      {
        m_Tests.reserve(sizeof...(tests));
        register_tests(std::forward<Tests>(tests)...);
      }
    }

    template<class concrete_test>
    void add_test(concrete_test&& test)
    {
      m_Tests.emplace_back(std::make_unique<concrete_test>(std::forward<concrete_test>(test)));
    }

    [[nodiscard]]
    auto execute(const bool writeFiles, const concurrency_mode mode) -> results ;

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }

    [[nodiscard]]
    bool empty() const noexcept { return m_Tests.empty(); }
  private:
    std::vector<std::unique_ptr<test>> m_Tests{};
    std::string m_Name{};

    [[nodiscard]]
    std::filesystem::path diagnostics_filename() const;

    [[nodiscard]]
    static std::filesystem::path test_summary_filename(const test& t, const bool writeFiles);

    static void write_summary_to_file(const log_summary& summary, std::set<std::string>& record);

    template<class Test, class... Tests>
    void register_tests(Test&& t, Tests&&... tests)
    {
      m_Tests.emplace_back(std::make_unique<Test>(std::forward<Test>(t)));

      if constexpr (sizeof...(Tests) > 0)
        register_tests(std::forward<Tests>(tests)...);
    }

    class summary_writer
    {
    public:
      void to_file(const std::filesystem::path& filename, const log_summary& summary);
    private:
      std::set<std::filesystem::path> m_Record{};
    };
  };  

  [[nodiscard]]
  std::filesystem::path get_output_path(std::string_view subDirectory);

  [[nodiscard]]
  std::string summarize(const test_family::summary& summary, const log_verbosity suppression, std::string_view indent_0, std::string_view indent_1);
}
