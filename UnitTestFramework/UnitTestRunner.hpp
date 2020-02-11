////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestRunner.hpp
    \brief Helper for running unit tests from the command line.
*/

#include "UnitTestFamily.hpp"

#include <map>

namespace sequoia::unit_testing
{
  template<class Iter> void pad_right(Iter begin, Iter end, std::string_view suffix)
  {
    auto maxIter{
      std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{maxIter->size()};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s += std::string(maxChars - s.size(), ' ') += std::string{suffix};
    }
  }  
      
  template<class Iter> void pad_left(Iter begin, Iter end, const std::size_t minChars)
  {
    auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{std::max(maxIter->size(), minChars)};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s = std::string(maxChars - s.size(), ' ') + s;
    }
  }

  class unit_test_runner
  {
  public:
    unit_test_runner(int argc, char** argv);

    unit_test_runner(const unit_test_runner&) = delete;
    unit_test_runner(unit_test_runner&&)      = default;

    template<class Test, class... Tests>
    void add_test_family(std::string_view name, Test&& test, Tests&&... tests)
    {
      if(m_SelectedSources.empty())
      {
        if(mark_family(name))
        {
          m_Families.emplace_back(name, std::forward<Test>(test), std::forward<Tests>(tests)...);
        }
      }
      else
      {
        test_family f{name};
        add_tests(f, std::forward<Test>(test), std::forward<Tests>(tests)...);
        if(!f.empty())
        {
          m_Families.push_back(std::move(f));
          mark_family(name);
        }
        else if(!m_SelectedFamilies.empty())
        {
          if(mark_family(name))
          {
            m_Families.emplace_back(name, std::forward<Test>(test), std::forward<Tests>(tests)...);
          }
        }
      }
    } 

    void execute();

    [[nodiscard]]
    concurrency_mode concurrency() const noexcept { return m_ConcurrencyMode; }
  private:    
    struct nascent_test
    {
      nascent_test(std::string dir, std::string qualifiedName);
      
      std::string directory, qualified_class_name, class_name;
    };    

    enum class file_comparison {failed, same, different};    

    std::vector<test_family> m_Families{};
    std::map<std::string, bool, std::less<>> m_SelectedFamilies{}, m_SelectedSources{};
    std::vector<nascent_test> m_NewFiles{};
    
    bool m_Verbose{}, m_Pause{}, m_WriteFiles{true};

    concurrency_mode m_ConcurrencyMode{concurrency_mode::serial};

    const static std::array<std::string_view, 5> st_TestNameStubs;

    bool mark_family(std::string_view name);

    [[nodiscard]]
    test_family::summary process_family(const test_family::results& results);

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void check_argument_consistency();

    void run_diagnostics();

    void run_tests();

    void check_for_missing_tests();

    template<class Test, class... Tests>
    void add_tests(test_family& f, Test&& test, Tests&&... tests)
    {
      auto i{m_SelectedSources.find(test.source_file_name())};
      if(i != m_SelectedSources.end())
      {
        f.add_test(std::forward<Test>(test));
        i->second = true;
      }
      
      if constexpr(sizeof...(Tests) > 0) add_tests(f, std::forward<Tests>(tests)...);
    }

    [[nodiscard]]
    static std::string to_camel_case(std::string text);

    [[nodiscard]]
    static std::string stringify(concurrency_mode mode);

    static void replace_all(std::string& text, std::string_view from, const std::string& to);

    [[nodiscard]]
    static bool file_exists(const std::string& path) noexcept;

    template<class Iter>
    static void create_files(Iter beginFiles, Iter endFiles, std::string_view message, const bool overwrite);

    static void create_file(const nascent_test& data, std::string_view partName, const bool overwrite);

    static auto compare_files(std::string_view referenceFile, std::string_view generatedFile) -> file_comparison;

    template<class Iter>
    static void compare_files(Iter beginFiles, Iter endFiles, std::string_view message);

    static void compare_files(const nascent_test& data, std::string_view partName);

    static void false_positive_check(const nascent_test& data);
    
    static void argument_processing_diagnostics();
  };
}
