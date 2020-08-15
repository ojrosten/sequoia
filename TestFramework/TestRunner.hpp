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

  [[nodiscard]]
  std::string report_file_issue(const std::filesystem::path& file, std::string_view description);

  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file);

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file);

  template<class Fn>
    requires invocable<Fn, std::filesystem::path>
  void create_file(const std::filesystem::path& source, const std::filesystem::path& target, Fn action)
  {
    namespace fs = std::filesystem;

    fs::copy_file(source, target, fs::copy_options::overwrite_existing);
    action(target);
  }

  [[nodiscard]]
  std::string compare_files(const std::filesystem::path& file, const std::filesystem::path& prediction, const test_mode mode, indentation ind);


  /*! \brief Holds data for the automated creation of new tests

   */

  class nascent_test
  {
  public:
    nascent_test(std::filesystem::path dir, std::string family, std::string qualifiedName);

    [[nodiscard]]
    std::string create_file(std::string_view partName, const std::filesystem::copy_options options) const;

    [[nodiscard]]
    std::string compare_files(std::string_view partName, indentation ind) const;

    //[[nodiscard]]
    //std::string include() const;

    [[nodiscard]]
    std::string_view class_name() const noexcept { return m_ClassName; }
  private:
    std::filesystem::path m_Directory;
    std::string
      m_Family,
      m_QualifiedClassName,
      m_ClassName;

    void transform_file(const std::filesystem::path& file) const;
  };    

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
    struct sentinel
    {
      sentinel()
      {
        st_Indent.append("  ");
      }

      ~sentinel()
      {
        st_Indent.erase(st_Indent.size() - 2);
      }
    };
    
    std::vector<test_family> m_Families{};
    std::map<std::string, bool, std::less<>> m_SelectedFamilies{}, m_SelectedSources{};
    std::vector<nascent_test> m_NascentTests{};
    static inline std::string st_Indent{};

    [[nodiscard]]
    static indentation ind()
    {
      return indentation{st_Indent};
    }
    
    bool m_Verbose{}, m_Pause{}, m_WriteFiles{true};

    concurrency_mode m_ConcurrencyMode{concurrency_mode::serial};

    constexpr static std::array<std::string_view, 5> st_TestNameStubs {
      "TestingUtilities.hpp",
      "TestingDiagnostics.hpp",
      "TestingDiagnostics.cpp",
      "Test.hpp",
      "Test.cpp"
    };

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
    static std::string stringify(concurrency_mode mode);

    template<class Iter>
    [[nodiscard]]
    static std::string create_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message, const std::filesystem::copy_options options);

    template<class Iter>
    [[nodiscard]]
    static std::string compare_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message);

    template<class Fn>
      requires invocable<Fn, std::filesystem::path>
    [[nodiscard]]
    static std::string test_file_editing(std::string_view fileName, Fn action);

    static void false_positive_check();
    
    static void test_file_editing();

    static void test_creation();
  };

  [[nodiscard]]
  std::filesystem::path get_aux_path(std::string_view subDirectory);
}
