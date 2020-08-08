////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for running tests from the command line.
*/

#include "TestFamily.hpp"

#include <map>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string report_time(const test_family::summary& s);

  /*! \brief Consumes command-line arguments and holds all test families

      The various arguments have the following effect:

      test <name>                               : runs the specified test
      source <cpp file>                         : runs all tests in the specified cpp
      create <directory, namespace::class_name> : creates infrastructure for a new test
      
      --async-depth <[0-2]> / -ad: serial/family/test/deep

      --async    / -a: unless overridden runs families of test concurrently
      --verbose  / -v: provides detailed break down of test results 
      --nofiles  / -n: suppresses output of diagnostic files
      --pause    / -p: pauses execution until 'enter' is hit
      --recovery / -r: generates recovery file, which may help tracking down crashes

      If no arguments are specified, all tests are run, in serial, with the diagnostic
      files generated.

   */

  class test_runner
  {
  public:
    test_runner(int argc, char** argv);

    test_runner(const test_runner&) = delete;
    test_runner(test_runner&&)      = default;

    test_runner& operator=(const test_runner&) = delete;
    test_runner& operator=(test_runner&&)      = delete;

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
      nascent_test(std::filesystem::path dir, std::string qualifiedName);

      std::filesystem::path directory;
      std::string qualified_class_name, class_name;
    };    

    enum class file_comparison {failed, same, different};
    enum class false_positive_mode {yes, no};
    enum class overwrite_mode {yes, no};

    std::vector<test_family> m_Families{};
    std::map<std::string, bool, std::less<>> m_SelectedFamilies{}, m_SelectedSources{};
    std::vector<nascent_test> m_NascentTests{};
    
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

    template<class Iter>
    static void create_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message, const overwrite_mode overwrite);

    static void create_file(const nascent_test& data, std::string_view partName, const overwrite_mode overwrite);

    static void compare_files(const std::filesystem::path& referenceFile, const std::filesystem::path& generatedFile, const false_positive_mode falsePositive);

    template<class Iter>
    static void compare_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message);

    static void compare_files(const nascent_test& data, std::string_view partName);

    template<class Fn>
      requires invocable<Fn, std::filesystem::path>
    static void test_file_editing(std::string_view fileName, Fn action);

    static void test_file_editing();

    static void false_positive_check(const nascent_test& data);
    
    static void argument_processing_diagnostics();
  };

  [[nodiscard]]
  std::filesystem::path get_aux_path(std::string_view subDirectory);
}
