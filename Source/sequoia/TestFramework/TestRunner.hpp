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

#include "sequoia/TestFramework/TestFamily.hpp"
#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/Runtime/Factory.hpp"

#include <map>
#include <variant>
#include <iostream>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string report_time(const test_family::summary& s);

  class host_directory
  {
  public:
    struct paths
    {
      std::filesystem::path host_dir, header_path;
    };
    
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
    auto get(const std::filesystem::path& filename) const -> paths;

    [[nodiscard]]
    friend bool operator==(const host_directory&, const host_directory&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const host_directory&, const host_directory&) noexcept = default;
  private:
    struct generator
    {
      std::filesystem::path hostRepo;
      search_tree sourceRepo;

      [[nodiscard]]
      friend bool operator==(const generator&, const generator&) noexcept = default;

      [[nodiscard]]
      friend bool operator!=(const generator&, const generator&) noexcept = default;
    };

    std::variant<std::filesystem::path, generator> m_Data;
  };

  struct template_spec
  {    
    std::string species, symbol;
    
    [[nodiscard]]
    friend bool operator==(const template_spec&, const template_spec&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const template_spec&, const template_spec&) noexcept = default;
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
    const std::string& family() const noexcept { return m_Family; }

    void family(std::string name) { m_Family = std::move(name); }

    void host_dir(std::filesystem::path dir) { m_HostDirectory = host_directory{std::move(dir)}; }
    
    [[nodiscard]]
    const std::filesystem::path& header() const noexcept { return m_Header; }

    void header(std::filesystem::path h) { m_Header = std::move(h); }

    [[nodiscard]]
    const std::string& test_type() const noexcept { return m_TestType; }

    void test_type(std::string type) { m_TestType = std::move(type); }

    [[nodiscard]]
    const std::string& forename() const noexcept { return m_Forename; }

    void forename(std::string name) { m_Forename = std::move(name); }
    
    [[nodiscard]]
    const std::filesystem::path& host_dir() const noexcept { return m_HostDir; }

    [[nodiscard]]
    const std::filesystem::path& header_path() const noexcept { return m_HeaderPath; }

    [[nodiscard]]
    friend bool operator==(const nascent_test_base&, const nascent_test_base&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const nascent_test_base&, const nascent_test_base&) noexcept = default;
  protected:
    nascent_test_base(const nascent_test_base&)     = default;
    nascent_test_base(nascent_test_base&&) noexcept = default;
    nascent_test_base& operator=(const nascent_test_base&)     = default;
    nascent_test_base& operator=(nascent_test_base&&) noexcept = default;

    ~nascent_test_base() = default;

    void finalize();

    const std::string& camel_name() const noexcept { return m_CamelName; }

    void camel_name(std::string name) { m_CamelName = to_camel_case(std::move(name)); }

    struct file_data
    {
      std::filesystem::path output_file{};
      bool created{};
    };

    template<invocable<std::string&> FileTransformer>
    [[nodiscard]]
    auto create_file(const std::filesystem::path& codeTemplatesDir, std::string_view copyright, std::string_view inputNameStub, std::string_view nameEnding, FileTransformer transformer) const -> file_data;
  private:
    std::string m_Family{}, m_TestType{}, m_Forename{}, m_CamelName{};

    host_directory m_HostDirectory{};

    std::filesystem::path m_Header{}, m_HostDir{}, m_HeaderPath{};
  };

  class nascent_semantics_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    void qualified_name(std::string name) { m_QualifiedName = std::move(name); }

    void add_equivalent_type(std::string name) { m_EquivalentTypes.emplace_back(std::move(name)); }

    void finalize();

    [[nodiscard]]
    auto create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const -> file_data;
    
    [[nodiscard]]
    std::vector<std::string> constructors() const;

    [[nodiscard]]
    friend bool operator==(const nascent_semantics_test&, const nascent_semantics_test&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const nascent_semantics_test&, const nascent_semantics_test&) noexcept = default;

    constexpr static std::array<std::string_view, 5> stubs() noexcept
    {
      return {"TestingUtilities.hpp",
              "TestingDiagnostics.hpp",
              "TestingDiagnostics.cpp",
              "Test.hpp",
              "Test.cpp"};
    };
  private:

    std::string m_QualifiedName{};

    template_data m_TemplateData{};

    std::vector<std::string> m_EquivalentTypes{};

    void transform_file(std::string& text) const;
  };

  class nascent_allocation_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    constexpr static std::array<std::string_view, 2> stubs() noexcept
    {
      return {"AllocationTest.hpp",
              "AllocationTest.cpp"};
    };

    void finalize();

    [[nodiscard]]
    auto create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const -> file_data;
    
    [[nodiscard]]
    std::vector<std::string> constructors() const;
  private:
    void transform_file(std::string& text) const;
  }; 

  class nascent_behavioural_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    void finalize();

    [[nodiscard]]
    auto create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const->file_data;

    [[nodiscard]]
    std::vector<std::string> constructors() const;

    [[nodiscard]]
    friend bool operator==(const nascent_behavioural_test&, const nascent_behavioural_test&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const nascent_behavioural_test&, const nascent_behavioural_test&) noexcept = default;

    constexpr static std::array<std::string_view, 2> stubs() noexcept
    {
      return {"Test.hpp",
              "Test.cpp"};
    };
  private:
    void transform_file(std::string& text) const;
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

    test_runner(const test_runner&)     = delete;
    test_runner(test_runner&&) noexcept = default;

    test_runner& operator=(const test_runner&)     = delete;
    test_runner& operator=(test_runner&&) noexcept = default;

    template<class Test, class... Tests>
    void add_test_family(std::string_view name, Test&& test, Tests&&... tests)
    {
      const bool done{
        [this, name](Test&& test, Tests&&... tests){
          if(!m_SelectedSources.empty())
          {
            test_family f{name, m_TestRepo, m_TestMaterialsRepo, m_OutputDir, m_Recovery};
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
          m_Families.emplace_back(name, m_TestRepo, m_TestMaterialsRepo, m_OutputDir, m_Recovery,
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
    using source_list = std::vector<std::pair<std::filesystem::path, bool>>;

    struct init_data
    {
      std::string copyright;
      std::filesystem::path location;
    };

    using creation_factory
      = runtime::factory<nascent_semantics_test, nascent_allocation_test, nascent_behavioural_test>;
    using vessel = typename creation_factory::vessel;

    std::vector<test_family> m_Families{};
    family_map m_SelectedFamilies{};
    source_list m_SelectedSources{};
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

    recovery_paths m_Recovery;

    std::ostream* m_Stream;

    output_mode m_OutputMode{output_mode::standard};
    update_mode m_UpdateMode{update_mode::none};
    concurrency_mode m_ConcurrencyMode{concurrency_mode::serial};

    std::ostream& stream() noexcept { return *m_Stream; }
 
    bool mark_family(std::string_view name);

    void process_args(int argc, char** argv);

    [[nodiscard]]
    test_family::summary process_family(const test_family::results& results);

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void check_argument_consistency() const;

    void run_tests();

    void check_for_missing_tests();

    [[nodiscard]]
    auto find_filename(const std::filesystem::path& filename) -> source_list::iterator;

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

    template<class Nascent>
    [[nodiscard]]
    std::string create_file(const Nascent& nascent, std::string_view stub) const;

    [[nodiscard]]
    std::string create_files() const;

    void init_project(std::string_view copyright, const std::filesystem::path& path);

    [[nodiscard]]
    bool mode(output_mode m) const noexcept
    {
      return (m_OutputMode & m) == m;
    }

    void generate_test_main(std::string_view copyright, const std::filesystem::path& path) const;

    void generate_build_system_files(const std::filesystem::path& path, std::string_view filename, std::string_view pattern) const;
 };
}
