////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
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
#include <variant>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string report_time(const test_family::summary& s);

  template<class Fn>
    requires invocable<Fn, std::filesystem::path>
  void create_file(const std::filesystem::path& source, const std::filesystem::path& target, Fn action)
  {
    namespace fs = std::filesystem;

    fs::copy_file(source, target, fs::copy_options::overwrite_existing);
    action(target);
  }

  class host_directory
  {
  public:
    host_directory(std::filesystem::path dir)
      : m_Data{std::move(dir)}
    {
      namespace fs = std::filesystem;
      fs::create_directories(std::get<fs::path>(m_Data));
    }

    host_directory(std::filesystem::path hostRepo, search_tree sourceRepo)
      : m_Data{generator{std::move(hostRepo), std::move(sourceRepo)}}
    {
      namespace fs = std::filesystem;
      fs::create_directories(std::get<generator>(m_Data).hostRepo);
    }

    std::filesystem::path get([[maybe_unused]] std::string_view filename) const;
  private:
    struct generator
    {
      std::filesystem::path hostRepo;
      search_tree sourceRepo;
    };

    std::variant<std::filesystem::path, generator> m_Data;
  };

  /*! \brief Holds data for the automated creation of new tests

   */

  class nascent_test
  {
  public:
    nascent_test(std::string_view testType, std::string_view qualifiedName, std::vector<std::string> equivalentTypes, const host_directory& hostDir, std::string_view overriddenFamily="", std::string_view overriddenClassHeader="");

    [[nodiscard]]
    std::filesystem::path create_file(std::string_view copyright, std::string_view partName, const std::filesystem::copy_options options) const;

    [[nodiscard]]
    std::string_view family() const noexcept { return m_Family; }

    [[nodiscard]]
    std::string_view class_name() const noexcept { return m_RawClassName; }
  private:
    struct template_spec { std::string parameter, name; };
    using template_data = std::vector<template_spec>;

    std::string
      m_Family{},
      m_QualifiedClassName{},
      m_RawClassName{},
      m_TestType{},
      m_ClassHeader{};
    
    std::filesystem::path m_HostDirectory{};

    template_data m_TemplateData{};

    std::vector<std::string> m_EquivalentTypes{};

    void transform_file(const std::filesystem::path& file, std::string_view copyright) const;

    [[nodiscard]]
    static auto generate_template_data(std::string_view str) -> template_data;

    [[nodiscard]]
    static auto generate_template_spec(std::string_view str) -> template_spec;
  };

  /*! \brief Consumes command-line arguments and holds all test families

      The various arguments have the following effect:

      test <name>                               : runs the specified test
      source <cpp file>                         : runs all tests in the specified cpp
      create <test type, qualified::class_name<class T>, test host directory> 
                                                : creates infrastructure for a new test
      
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
    test_runner(int argc, char** argv, std::string_view copyright, std::filesystem::path testMain, std::filesystem::path hashIncludeTarget, std::filesystem::path sourceRepo, std::filesystem::path testRepo, std::filesystem::path testMaterialsRepo, std::filesystem::path outputDir);

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
          m_Families.emplace_back(name, m_TestRepo, m_TestMaterialsRepo, m_OutputDir, std::forward<Test>(test), std::forward<Tests>(tests)...);
        }
      }
      else
      {
        test_family f{name, m_TestRepo, m_TestMaterialsRepo, m_OutputDir};
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
            m_Families.emplace_back(name, m_TestRepo, m_TestMaterialsRepo, m_OutputDir, std::forward<Test>(test), std::forward<Tests>(tests)...);
          }
        }
      }
    } 

    void execute();

    [[nodiscard]]
    concurrency_mode concurrency() const noexcept { return m_ConcurrencyMode; }
  private:
    class sentinel
    {
    public:
      using size_type = std::string::size_type;
      
      sentinel(test_runner& runner)
        : m_Runner{runner}
        , m_Indentation(2*(++m_Runner.m_Depth), ' ')
      {
      }
      
      [[nodiscard]]
      std::string indent(std::string_view s) const
      {
        return testing::indent(s, indentation{m_Indentation});
      }

      std::string& append_indented(std::string& s1, std::string_view s2)
      {
        return testing::append_indented(s1, s2, indentation{m_Indentation});
      }

      ~sentinel()
      {
        --m_Runner.m_Depth;
      }
    private:
      test_runner& m_Runner;
      std::string m_Indentation;
    };

    struct messages
    {
      std::string creation, comparison;
    };

    using selection_map = std::map<std::string, bool, std::less<>>;

    std::vector<test_family> m_Families{};
    selection_map m_SelectedFamilies{}, m_SelectedSources{};
    std::vector<nascent_test> m_NascentTests{};
    std::string m_Copyright{};
    search_tree m_SourceSearchTree;
    std::filesystem::path
      m_TestMain{},
      m_HashIncludeTarget{},
      m_TestRepo{},
      m_TestMaterialsRepo{},
      m_OutputDir{};
    sentinel::size_type m_Depth{};
    
    bool m_Verbose{}, m_Pause{};
    output_mode m_OutputMode{output_mode::write_files};

    concurrency_mode m_ConcurrencyMode{concurrency_mode::serial};

    constexpr static std::array<std::string_view, 5> st_TestNameStubs {
      "TestingUtilities.hpp",
      "TestingDiagnostics.hpp",
      "TestingDiagnostics.cpp",
      "Test.hpp",
      "Test.cpp"
    };

    void process_args(int argc, char** argv);

    bool mark_family(std::string_view name);

    [[nodiscard]]
    test_family::summary process_family(const test_family::results& results);

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void check_argument_consistency() const;

    void run_tests();

    void check_for_missing_tests() const;

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

    void report(std::string_view prefix, std::string_view message);

    template<class Iter, class Filter>
      requires invocable<Filter, test_runner::messages>
    void report(std::string_view prefix, Iter begin, Iter end, Filter filter);

    [[nodiscard]]
    static std::string stringify(concurrency_mode mode);

    template<class Iter, class Fn>
    [[nodiscard]]
    static std::string process_nascent_tests(Iter beginNascentTests, Iter endNascentTests, Fn fn);

    template<class Iter>
    [[nodiscard]]
    std::string create_files(Iter beginNascentTests, Iter endNascentTests, const std::filesystem::copy_options options) const;
  };
}
