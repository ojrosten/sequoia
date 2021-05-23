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
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/Runtime/Factory.hpp"

#include <map>
#include <iostream>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string report_time(const test_family::summary& s);

  struct repositories
  {
    explicit repositories(const std::filesystem::path& projectRoot);

    [[nodiscard]]
    static std::filesystem::path source_path(const std::filesystem::path& projectRoot);

    [[nodiscard]]
    friend bool operator==(const repositories&, const repositories&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const repositories&, const repositories&) noexcept = default;

    std::filesystem::path
      project_root{},
      source{},
      tests{},
      test_materials{},
      output{};
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
  std::string to_string(const template_data& data);

  [[nodiscard]]
  template_data generate_template_data(std::string_view str);

  [[nodiscard]]
  template_spec generate_template_spec(std::string_view str);

  void set_top_copyright(std::string& text, std::string_view copyright);

  class nascent_test_base
  {
  public:
    enum class gen_source_option {no, yes};

    nascent_test_base(std::filesystem::path testMainDir, repositories repos)
      : m_TestMainDir{std::move(testMainDir)}
      , m_Repos{std::move(repos)}
    {}

    [[nodiscard]]
    const std::string& family() const noexcept { return m_Family; }

    void family(std::string name) { m_Family = std::move(name); }

    [[nodiscard]]
    const std::filesystem::path& header() const noexcept { return m_Header; }

    void header(std::filesystem::path h) { m_Header = std::move(h); }

    [[nodiscard]]
    const std::string& test_type() const noexcept { return m_TestType; }

    void test_type(std::string type) { m_TestType = std::move(type); }

    [[nodiscard]]
    const std::string& forename() const noexcept { return m_Forename; }

    void forename(std::string name) { m_Forename = std::move(name); }

    void generate_source_files(gen_source_option opt)
    {
      m_SourceOption = opt;
    }

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

    [[nodiscard]]
    const std::filesystem::path& test_main_dir() const noexcept
    {
      return m_TestMainDir;
    }

    [[nodiscard]]
    const repositories& repos() const noexcept
    {
      return m_Repos;
    }

    std::filesystem::path build_source_path(const std::filesystem::path& filename,
                                            const std::vector<std::string_view>& extensions) const;

    template<invocable_r<std::filesystem::path, std::filesystem::path> WhenAbsent>
    void finalize(WhenAbsent fn);

    const std::string& camel_name() const noexcept { return m_CamelName; }

    void camel_name(std::string name);

    struct file_data
    {
      std::filesystem::path output_file{};
      bool created{};
    };

    template<invocable<std::string&> FileTransformer>
    [[nodiscard]]
    auto create_file(const std::filesystem::path& codeTemplatesDir, std::string_view copyright, std::string_view inputNameStub, std::string_view nameEnding, FileTransformer transformer) const -> file_data;

    void set_cpp(const std::filesystem::path& headerPath, std::string_view nameSpace) const;

    [[nodiscard]]
    const std::string& code_indent() const noexcept
    {
      return m_CodeIndent;
    }
  private:
    std::string m_Family{}, m_TestType{}, m_Forename{}, m_CamelName{};

    repositories m_Repos;

    std::filesystem::path m_TestMainDir{}, m_Header{}, m_HostDir{}, m_HeaderPath{};

    gen_source_option m_SourceOption{};

    std::string m_CodeIndent{"  "};

    void on_source_path_error(const std::vector<std::string_view>& extensions) const;

    void finalize_family();

    void finalize_header(const std::filesystem::path& sourcePath);
  };

  class nascent_semantics_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    void qualified_name(std::string name) { m_QualifiedName = std::move(name); }

    void add_equivalent_type(std::string name) { m_EquivalentTypes.emplace_back(std::move(name)); }

    void source_dir(std::filesystem::path dir) { m_SourceDir = std::move(dir); }

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

    std::filesystem::path m_SourceDir{};

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
      return {"Test.hpp", "Test.cpp"};
    };

    void set_namespace(std::string n) { m_Namespace = std::move(n); }
  private:
    void transform_file(std::string& text) const;

    std::string m_Namespace;
   };

  /*! \brief Consumes command-line arguments and holds all test families

      If no arguments are specified, all tests are run, in serial, with the diagnostic
      files generated; run with --help for information on the various options

   */

  class test_runner
  {
  public:
    test_runner(int argc, char** argv, std::string_view copyright, std::filesystem::path testMainCpp, std::filesystem::path hashIncludeTarget, repositories repos, std::ostream& stream=std::cout);

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
            test_family f{name, m_Repos.tests, m_Repos.test_materials, m_Repos.output, m_Recovery};
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
          m_Families.emplace_back(name, m_Repos.tests, m_Repos.test_materials, m_Repos.output, m_Recovery,
                                  std::forward<Test>(test), std::forward<Tests>(tests)...);
        }
      }
    }

    void execute([[maybe_unused]] timer_resolution r={});

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
    std::filesystem::path
      m_TestMainCpp{},
      m_HashIncludeTarget{};
    repositories m_Repos;

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

    void generate_build_system_files(const std::filesystem::path& root) const;
 };
}
