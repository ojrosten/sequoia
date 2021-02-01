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
#include "CommandLineArguments.hpp"
#include "Factory.hpp"

#include <map>
#include <variant>
#include <iostream>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string report_time(const test_family::summary& s);

  template<invocable<std::filesystem::path> Fn>
  void create_file(const std::filesystem::path& source, const std::filesystem::path& target, Fn action)
  {
    namespace fs = std::filesystem;

    fs::copy_file(source, target, fs::copy_options::overwrite_existing);
    action(target);
  }

  class host_directory
  {
  public:
    host_directory() = default;

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

    [[nodiscard]]
    std::filesystem::path get([[maybe_unused]] const std::filesystem::path& filename) const;
  private:
    struct generator
    {
      std::filesystem::path hostRepo;
      search_tree sourceRepo;
    };

    std::variant<std::filesystem::path, generator> m_Data;
  };

  struct template_spec
  {
    [[nodiscard]]
    friend bool operator==(const template_spec&, const template_spec&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const template_spec&, const template_spec&) noexcept = default;
    
    std::string species, symbol;
  };


  using template_data = std::vector<template_spec>;
  
  [[nodiscard]]
  template_data generate_template_data(std::string_view str);

  [[nodiscard]]
  template_spec generate_template_spec(std::string_view str);
  
  void set_top_copyright(std::string& text, std::string_view copyright);

  class nascent_test_base
  {
  public:    
    nascent_test_base(std::filesystem::path testRepo, search_tree sourceTree)
      : m_HostDirectory{std::move(testRepo), std::move(sourceTree)}
    {}

    [[nodiscard]]
    std::string_view family() const noexcept { return m_Family; }

    void family(std::string name) { m_Family = std::move(name); }

    [[nodiscard]]
    std::filesystem::path host_dir() const noexcept { return m_HostDirectory.get(m_Header); }

    void host_dir(std::filesystem::path dir) { m_HostDirectory = host_directory{std::move(dir)}; }
    
    [[nodiscard]]
    const std::filesystem::path& header() const noexcept { return m_Header; }

    void header(std::string h) { m_Header = std::move(h); }

    [[nodiscard]]
    std::string_view test_type() const noexcept { return m_TestType; }

    void test_type(std::string type) { m_TestType = std::move(type); }

    [[nodiscard]]
    std::string_view forename() const noexcept { return m_Forename; }
    
    void finalize(std::string_view camelName);
  protected:
    nascent_test_base(const nascent_test_base&)     = default;
    nascent_test_base(nascent_test_base&&) noexcept = default;
    nascent_test_base& operator=(const nascent_test_base&)     = default;
    nascent_test_base& operator=(nascent_test_base&&) noexcept = default;

    ~nascent_test_base() = default;
   
    void forename(std::string name) { m_Forename = std::move(name); }
  private:
    std::string m_Family{}, m_TestType{}, m_Forename{};

    host_directory m_HostDirectory{};
    
    std::filesystem::path m_Header{};
  };

  class nascent_semantics_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    void qualified_name(std::string name) { m_QualifiedName = std::move(name); }

    void add_equivalent_type(std::string name) { m_EquivalentTypes.emplace_back(std::move(name)); }

    void finalize();

    [[nodiscard]]
    std::filesystem::path create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view partName, std::filesystem::copy_options options) const;

  private:

    std::string m_QualifiedName{};

    template_data m_TemplateData{};

    std::vector<std::string> m_EquivalentTypes{};

    void transform_file(const std::filesystem::path& file, std::string_view copyright) const;
  };

  class nascent_behavioural_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    void finalize();
  };

  struct repositories
  {
    explicit repositories(const std::filesystem::path& projectRoot)
      : source{projectRoot/"Source"}
      , tests{projectRoot/"Tests"}
      , test_materials{projectRoot/"TestMaterials"}
      , output{projectRoot/"output"}
    {}
    
    std::filesystem::path
      source{},
      tests{},
      test_materials{},
      output{};
  };

  /*! \brief Consumes command-line arguments and holds all test families

      If no arguments are specified, all tests are run, in serial, with the diagnostic
      files generated; run with --help for information on the various options

   */

  class test_runner
  {    
  public:    
    test_runner(int argc, char** argv, std::string_view copyright, std::filesystem::path testMain, std::filesystem::path hashIncludeTarget, repositories repos, std::ostream& stream=std::cout);

    test_runner(const test_runner&) = delete;
    test_runner(test_runner&&)      = default;

    test_runner& operator=(const test_runner&) = delete;
    test_runner& operator=(test_runner&&)      = delete;

    template<class Test, class... Tests>
    void add_test_family(std::string_view name, Test&& test, Tests&&... tests)
    {
      const bool done{
        [this, name](Test&& test, Tests&&... tests){
          if(!m_SelectedSources.empty())
          {
            test_family f{name, m_TestRepo, m_TestMaterialsRepo, m_OutputDir};
            add_tests(f, std::forward<Test>(test), std::forward<Tests>(tests)...);
            if(!f.empty())
            {
              m_Families.push_back(std::move(f));
              mark_family(name);
              return true;
            }
          }

          return false;
        }(std::forward<Test>(test), std::forward<Tests>(tests)...)
      };
      
      if(!done && (m_SelectedSources.empty() || !m_SelectedFamilies.empty()))
      {
        if(mark_family(name))
        {
          m_Families.emplace_back(name, m_TestRepo, m_TestMaterialsRepo, m_OutputDir,
                                  std::forward<Test>(test), std::forward<Tests>(tests)...);
        }
      }
    } 

    void execute();

    [[nodiscard]]
    concurrency_mode concurrency() const noexcept { return m_ConcurrencyMode; }

  private:
    struct test_creator
    {        
      test_creator(std::string type, std::string subType, test_runner& r)
        : genus{std::move(type)}
        , species{std::move(subType)}
        , runner{r}
      {}

      void operator()(const parsing::commandline::param_list& args);   
        
      std::string genus, species;
      test_runner& runner;
    };

    friend test_creator;
    
    using family_map = std::map<std::string, bool, std::less<>>;
    using source_map = std::map<std::filesystem::path, bool>;

    struct init_data
    {
      std::string copyright;
      std::filesystem::path location;
    };

    using creation_factory = runtime::factory<nascent_semantics_test>;
    using vessel = typename creation_factory::vessel;

    static decltype(auto) get_family(const vessel& v);
    static decltype(auto) get_forename(const vessel& v);


    std::vector<test_family> m_Families{};
    family_map m_SelectedFamilies{};
    source_map m_SelectedSources{};
    std::vector<vessel> m_NascentTests{};
    std::string m_Copyright{};
    search_tree m_SourceSearchTree;
    std::filesystem::path
      m_ProjectRoot{},
      m_TestMain{},
      m_HashIncludeTarget{},
      m_TestRepo{},
      m_TestMaterialsRepo{},
      m_OutputDir{};
    
    std::ostream& m_Stream;

    output_mode m_OutputMode{output_mode::none};
    concurrency_mode m_ConcurrencyMode{concurrency_mode::serial};
    
    bool mark_family(std::string_view name);

    constexpr static std::array<std::string_view, 5> st_TestNameStubs {
      "TestingUtilities.hpp",
      "TestingDiagnostics.hpp",
      "TestingDiagnostics.cpp",
      "Test.hpp",
      "Test.cpp"
    };

    void process_args(int argc, char** argv);

    [[nodiscard]]
    test_family::summary process_family(const test_family::results& results);

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void check_argument_consistency() const;

    void run_tests();

    void check_for_missing_tests();

    [[nodiscard]]
    auto find_filename(const std::filesystem::path& filename) -> source_map::iterator;

    template<class Test, class... Tests>
    void add_tests(test_family& f, Test&& test, Tests&&... tests)
    {
      auto i{find_filename(test.source_filename())};
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
    std::string process_nascent_semantics_tests(Iter beginNascentTests, Iter endNascentTests, Fn fn) const;

    template<class Iter>
    [[nodiscard]]
    std::string create_files(Iter beginNascentTests, Iter endNascentTests, const std::filesystem::copy_options options) const;

    void init_project(std::string_view copyright, const std::filesystem::path& path) const;

    [[nodiscard]]
    bool mode(output_mode m) const noexcept
    {
      return (m_OutputMode & m) == m;
    }

    void generate_test_main(std::string_view copyright, const std::filesystem::path& path) const;

    void generate_make_file(const std::filesystem::path& path) const;
  };
}
