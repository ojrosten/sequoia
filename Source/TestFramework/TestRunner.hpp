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
#include <iostream>

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

  /*! \brief data supplied from the commandline for creating new tests */

  struct creation_data
  {
    explicit creation_data(host_directory hostDir)
      : host{std::move(hostDir)}
      , defaultHost{host}
    {}
      
    creation_data(std::filesystem::path testRepo, search_tree sourceTree)
      : host{std::move(testRepo), std::move(sourceTree)}
      , defaultHost{host}
    {}

    class sentinel
    {
    public:
      sentinel(creation_data& creationData)
        : m_CreationData{creationData}
      {}

      ~sentinel()
      {
        m_CreationData = creation_data{m_CreationData.defaultHost};
      }
    private:
      creation_data& m_CreationData;
    };
      
    std::vector<std::string> equivalentTypes{};
    host_directory host, defaultHost;
    std::string family{}, classHeader{};
  };

  /*! \brief Holds data for the automated creation of new tests */
  class nascent_test
  {
  public:
    nascent_test(std::string_view testType, std::string_view qualifiedName, creation_data data);

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

  struct repositories
  {
    std::filesystem::path
      source{sibling_path("Source")},
      tests{sibling_path("Tests")},
      test_materials{sibling_path("TestMaterials")},
      output{sibling_path("output")};
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
      --recovery / -r: generates recovery file, which may help tracking down crashes

      If no arguments are specified, all tests are run, in serial, with the diagnostic
      files generated.

   */

  class test_runner
  {
  public:    
    test_runner(int argc, char** argv, std::string_view copyright, std::filesystem::path testMain, std::filesystem::path hashIncludeTarget, repositories repos=repositories{}, std::ostream& stream=std::cout);

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
    
    std::ostream& m_Stream;
    
    bool m_Verbose{};
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

    void report(std::string_view prefix, std::string_view message);

    [[nodiscard]]
    static std::string stringify(concurrency_mode mode);

    template<class Iter, class Fn>
    [[nodiscard]]
    std::string process_nascent_tests(Iter beginNascentTests, Iter endNascentTests, Fn fn) const;

    template<class Iter>
    [[nodiscard]]
    std::string create_files(Iter beginNascentTests, Iter endNascentTests, const std::filesystem::copy_options options) const;
  };
}
