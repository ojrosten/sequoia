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
#include "sequoia/TextProcessing/Indent.hpp"
#include "sequoia/Runtime/Factory.hpp"

#include <map>
#include <iostream>
#include <chrono>

namespace sequoia::testing
{
  class shell_command
  {
  public:
    enum class append_mode{no, yes};

    shell_command() = default;

    shell_command(std::string cmd) : m_Command{std::move(cmd)}
    {}

    shell_command(std::string_view preamble, std::string cmd, const std::filesystem::path& output, append_mode app = append_mode::no);

    [[nodiscard]]
    bool empty() const noexcept
    {
      return m_Command.empty();
    }

    [[nodiscard]]
    friend bool operator==(const shell_command&, const shell_command&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const shell_command&, const shell_command&) noexcept = default;

    [[nodiscard]]
    friend shell_command operator&&(const shell_command& lhs, const shell_command& rhs)
    {
      return rhs.empty() ? lhs :
             lhs.empty() ? rhs :
                           std::string{lhs.m_Command}.append("&&").append(rhs.m_Command);
    }

    [[nodiscard]]
    friend shell_command operator&&(const shell_command& lhs, std::string rhs)
    {
      return lhs && shell_command{rhs};
    }

    friend void invoke(const shell_command& cmd);
  private:
    std::string m_Command;

    shell_command(std::string cmd, const std::filesystem::path& output, append_mode app);

  };

  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir);

  [[nodiscard]]
  shell_command cmake_cmd(const std::filesystem::path& buildDir, const std::filesystem::path& output);

  [[nodiscard]]
  shell_command build_cmd(const std::filesystem::path& buildDir, const std::filesystem::path& output);

  [[nodiscard]]
  shell_command git_first_cmd(const std::filesystem::path& root, const std::filesystem::path& output);

  [[nodiscard]]
  shell_command launch_cmd(const std::filesystem::path& root, const std::filesystem::path& buildDir);

  [[nodiscard]]
  std::string report_time(const test_family::summary& s);

  [[nodiscard]]
  bool handle_as_ref(std::string_view type);

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

    nascent_test_base(project_paths paths, std::string copyright, indentation codeIndent, std::ostream& stream)
      : m_Paths{std::move(paths)}
      , m_Copyright{std::move(copyright)}
      , m_CodeIndent{codeIndent}
      , m_Stream{&stream}
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
    const project_paths& paths() const noexcept
    {
      return m_Paths;
    }

    [[nodiscard]]
    std::filesystem::path build_source_path(const std::filesystem::path& filename) const;

    template<invocable_r<std::filesystem::path, std::filesystem::path> WhenAbsent, std::size_t N, invocable<std::string&> FileTransformer>
    void finalize(WhenAbsent fn,
                  const std::array<std::string_view, N>& stubs,
                  const std::vector<std::string>& constructors,
                  std::string_view nameStub,
                  FileTransformer transformer);

    [[nodiscard]]
    const std::string& camel_name() const noexcept { return m_CamelName; }

    void camel_name(std::string name);

    void set_cpp(const std::filesystem::path& headerPath, std::string_view copyright, std::string_view nameSpace);

    [[nodiscard]]
    const indentation& code_indent() const noexcept
    {
      return m_CodeIndent;
    }

    const std::string& copyright() const noexcept
    {
      return m_Copyright;
    }

    [[nodiscard]]
    std::ostream& stream() noexcept { return *m_Stream; }

    void finalize_family(std::string_view fallbackIngredient);
  private:
    constexpr static std::array<std::string_view, 3> st_HeaderExtensions{".hpp", ".h", ".hxx"};

    project_paths m_Paths;
    std::string m_Copyright{};
    indentation m_CodeIndent{"  "};
    std::ostream* m_Stream;

    std::string m_Family{}, m_TestType{}, m_Forename{}, m_CamelName{};
    std::filesystem::path m_Header{}, m_HostDir{}, m_HeaderPath{};
    gen_source_option m_SourceOption{};

    void on_source_path_error() const;

    void finalize_header(const std::filesystem::path& sourcePath);

    template<invocable<std::string&> FileTransformer>
    [[nodiscard]]
    std::string create_file(std::string_view inputNameStub, std::string_view nameEnding, FileTransformer transformer) const;

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

    void set_header_text(std::string& text, std::string_view copyright, std::string_view nameSpace) const;

    [[nodiscard]]
    std::filesystem::path when_header_absent(const std::filesystem::path& filename, const std::string& nameSpace);
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

    [[nodiscard]]
    std::filesystem::path when_header_absent(const std::filesystem::path& filename);
   };

  /*! \brief Consumes command-line arguments and holds all test families

      If no arguments are specified, all tests are run, in serial, with the diagnostic
      files generated; run with --help for information on the various options

   */

  class test_runner
  {
  public:
    test_runner(int argc, char** argv, std::string copyright, project_paths paths, std::string codeIndent="  ", std::ostream& stream=std::cout);

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
            test_family f{name, m_Paths.tests(), m_Paths.test_materials(), m_Paths.output(), m_Recovery};
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

      if(!done && ( (m_SelectedSources.empty() && !pruned()) || !m_SelectedFamilies.empty() ))
      {
        if(mark_family(name))
        {
          m_Families.emplace_back(name, m_Paths.tests(), m_Paths.test_materials(), m_Paths.output(), m_Recovery,
                                  std::forward<Test>(test), std::forward<Tests>(tests)...);
        }
      }
    }

    void execute([[maybe_unused]] timer_resolution r={});

    [[nodiscard]]
    concurrency_mode concurrency() const noexcept { return m_ConcurrencyMode; }

  private:
    enum class build_invocation { no = 0, yes, launch_ide };

    struct nascent_test_data
    {
      nascent_test_data(std::string type, std::string subType, test_runner& r)
        : genus{std::move(type)}
        , species{std::move(subType)}
        , runner{r}
      {}

      void operator()(const parsing::commandline::arg_list& args);

      std::string genus, species;
      test_runner& runner;
    };

    struct project_data
    {
      std::string copyright{};
      std::filesystem::path project_root{};
      indentation code_indent{"\t"};
      build_invocation do_build{build_invocation::launch_ide};
      std::filesystem::path output{};
    };

    struct time_stamps
    {
      using time_type = std::filesystem::file_time_type;
      using stamp = std::optional<time_type>;

      static auto from_file(const std::filesystem::path& stampFile) -> stamp;

      time_type current{std::chrono::file_clock::now()};
      stamp ondisk, executable;
    };

    struct prune_info
    {
      time_stamps stamps{};
      std::string include_cutoff{};
    };

    friend nascent_test_data;

    using family_map        = std::map<std::string, bool, std::less<>>;
    using source_list       = std::vector<std::pair<std::filesystem::path, bool>>;
    using creation_factory  = runtime::factory<nascent_semantics_test, nascent_allocation_test, nascent_behavioural_test>;
    using vessel            = typename creation_factory::vessel;

    std::string               m_Copyright{};
    project_paths             m_Paths;
    indentation               m_CodeIndent{"  "};
    std::ostream*             m_Stream;

    prune_info                m_PruneInfo{};
    std::vector<test_family>  m_Families{};
    family_map                m_SelectedFamilies{};
    source_list               m_SelectedSources{};
    std::vector<vessel>       m_NascentTests{};
    std::vector<project_data> m_NascentProjects{};
    recovery_paths            m_Recovery{};
    output_mode               m_OutputMode{output_mode::standard};
    update_mode               m_UpdateMode{update_mode::none};
    concurrency_mode          m_ConcurrencyMode{concurrency_mode::serial};

    std::ostream& stream() noexcept { return *m_Stream; }

    [[nodiscard]]
    bool pruned() const noexcept;

    bool mark_family(std::string_view name);

    void process_args(int argc, char** argv);

    [[nodiscard]]
    test_family::summary process_family(const test_family::results& results);

    [[nodiscard]]
    bool concurrent_execution() const noexcept { return m_ConcurrencyMode != concurrency_mode::serial; }

    void check_argument_consistency();

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

    /*template<class Nascent>
    [[nodiscard]]
    std::string create_file(const Nascent& nascent, std::string_view stub) const;*/

    void create_tests();

    void init_projects();

    [[nodiscard]]
    bool mode(output_mode m) const noexcept
    {
      return (m_OutputMode & m) == m;
    }

    void generate_test_main(std::string_view copyright, const std::filesystem::path& projRoot, indentation codeIndent) const;

    void generate_build_system_files(const std::filesystem::path& projRoot) const;
 };
}
