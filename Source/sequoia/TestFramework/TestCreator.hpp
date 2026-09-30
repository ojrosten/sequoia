////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for creating new tests, especially from the commandline.
  */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include "sequoia/Core/Object/Factory.hpp"
#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <array>
#include <optional>
#include <span>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  bool handle_as_ref(std::string_view type);

  struct template_spec
  {
    std::string species, symbol;

    [[nodiscard]]
    friend bool operator==(const template_spec&, const template_spec&) noexcept = default;
  };

  using template_data = std::vector<template_spec>;

  [[nodiscard]]
  std::string to_string(const template_data& data);

  [[nodiscard]]
  template_data generate_template_data(std::string_view str);

  [[nodiscard]]
  template_spec generate_template_spec(std::string_view str);

  std::string cmake_nascent_tests(const project_paths& projPaths);

  enum class nascent_test_flavour { standard, framework_diagnostics };

  /** \brief The name of the directory `sourceProject`.

      \throws std::runtime_error if `sourceProject` is not a directory, or its name is not an identifier.
   */
  [[nodiscard]]
  std::string project_namespace_for(const std::filesystem::path& sourceProject);

  enum class add_to_common_includes { no, yes };

  /** \brief A file written for the type under test, rather than for the test */
  struct companion_stub
  {
    std::string ending{};
    add_to_common_includes include{};
  };

  class nascent_test_base
  {
  public:
    enum class gen_source_option {no, yes};

    nascent_test_base(project_paths paths, std::string copyright, indentation codeIndent, std::ostream& stream)
      : m_Paths{std::move(paths)}
      , m_Copyright{std::move(copyright)}
      , m_CodeIndent{codeIndent}
      , m_Stream{&stream}
      , m_ProjectNamespace{project_namespace_for(m_Paths.source().project())}
    {}

    [[nodiscard]]
    nascent_test_flavour flavour() const noexcept { return m_Flavour; }

    void flavour(nascent_test_flavour f) { m_Flavour = f; }

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
    const std::string& surname() const noexcept { return m_Surname; }

    void surname(std::string name) { m_Surname = std::move(name); }

    /** \brief The test's full name, if one was given. A full name replaces the name derived from the
        forename and surname.
     */
    [[nodiscard]]
    const std::optional<std::string>& full_name() const noexcept { return m_FullName; }

    void full_name(std::string name) { m_FullName = std::move(name); }

    /** \brief An existing header that holds the `value_tester` for the type under test, if one was given.
        The test includes this header.
     */
    [[nodiscard]]
    const std::filesystem::path& testing_utilities() const noexcept { return m_TestingUtilities; }

    void testing_utilities(std::filesystem::path header) { m_TestingUtilities = std::move(header); }

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
    static std::vector<std::string> framework_diagnostics_stubs();
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

    /** \brief Creates the files, then registers the test classes in every main.

        Each companion file is named `type_file_stem()` followed by its stub. Each of the test's own files
        is named `test_file_stem()` followed by the extension of its stub.

        `whereAbsent` gives the path at which to generate the header under test, and `generate` writes the
        header there. Both are called only if the header cannot be found and its generation was requested.

        \throws std::runtime_error if
        -# The header under test cannot be found and is not to be generated;
        -# The testing utilities do not name exactly one file beneath the tests repository;
        -# A full name was given which cannot name the test.

        Every check is made before any file is written.
     */
    template<invocable_exact_r<std::filesystem::path, std::filesystem::path> WhereAbsent,
             std::invocable<std::filesystem::path> Generator,
             std::invocable<std::string&> FileTransformer>
    void finalize(WhereAbsent whereAbsent,
                  Generator generate,
                  const std::vector<companion_stub>& companionStubs,
                  const std::vector<std::string>& ownStubs,
                  const std::vector<std::string>& testClasses,
                  std::string_view nameStub,
                  FileTransformer transformer);

    /** \brief The full name if one was given, else `<forename>_<surname>`.

        The test's own files declare a class with this name. A framework-diagnostics test is the exception:
        its files declare two classes, each named from the forename and surname, and `test_name()` names
        the pair.
     */
    [[nodiscard]]
    std::string test_name() const;

    /** \brief The stem of the test's own files: the test's name in camel case */
    [[nodiscard]]
    std::string test_file_stem() const;

    /** \brief The path by which the test's header includes the testing utilities.

        The path is relative to the test's own directory if the testing utilities lie within that
        directory, and otherwise relative to the tests repository.
     */
    [[nodiscard]]
    std::string testing_utilities_include() const;

    /** \brief The stem of the files named for the type under test: the type's name in camel case */
    [[nodiscard]]
    const std::string& type_file_stem() const noexcept { return m_TypeFileStem; }

    /** \brief Sets the name of the type under test, and derives `type_file_stem()` from that name.

        If no header was given for the type, the header is taken to be `type_file_stem()` followed by `.hpp`.
     */
    void set_type_name(std::string_view name);

    void set_cpp(const std::filesystem::path& headerPath, std::string_view nameSpace);

    [[nodiscard]]
    const indentation& code_indent() const noexcept { return m_CodeIndent; }

    [[nodiscard]]
    const std::string& copyright() const noexcept { return m_Copyright; }

    [[nodiscard]]
    const std::string& project_namespace() const noexcept { return m_ProjectNamespace; }

    [[nodiscard]]
    std::ostream& stream() noexcept { return *m_Stream; }

    void make_common_replacements(std::string& text) const;
  private:
    constexpr static std::array<std::string_view, 3> st_HeaderExtensions{".hpp", ".h", ".hxx"};

    project_paths m_Paths;
    std::string m_Copyright{};
    indentation m_CodeIndent{"  "};
    std::ostream* m_Stream;

    nascent_test_flavour m_Flavour{nascent_test_flavour::standard};
    std::string
      m_TestType{},
      m_Forename{},
      m_Surname{},
      m_TypeFileStem{},
      m_ProjectNamespace{};
    std::optional<std::string> m_FullName{};
    std::filesystem::path m_Header{}, m_HostDir{}, m_HeaderPath{}, m_TestingUtilities{};
    gen_source_option m_SourceOption{};

    void on_source_path_error() const;

    void finalize_header(const std::filesystem::path& sourcePath);

    /** \brief Replaces the path to the testing utilities given on the commandline with the path of the
        file it names beneath the tests repository.

        \throws std::runtime_error if the given path names no such file, or several.
     */
    void locate_testing_utilities();

    /** \brief Checks that the full name can name the test.

        \throws std::runtime_error if the full name
        -# Is not an identifier;
        -# Names a test which is already registered;
        -# Would give one of the test's files the name of a companion file or of a file already present,
           ignoring case.
     */
    void check_full_name(std::span<const std::filesystem::path> companionFiles,
                         std::span<const std::filesystem::path> ownFiles) const;

    template<std::invocable<std::string&> FileTransformer>
    [[nodiscard]]
    std::string create_file(std::string_view inputNameStub,
                            std::string_view nameEnding,
                            const std::filesystem::path& outputFile,
                            add_to_common_includes include,
                            FileTransformer transformer) const;
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
    std::vector<std::string> test_classes() const;

    [[nodiscard]]
    friend bool operator==(const nascent_semantics_test&, const nascent_semantics_test&) noexcept = default;

    /** \brief The test's own files */
    [[nodiscard]]
    static std::vector<std::string> stubs();

    /** \brief The files for the type under test: the type's `value_tester` and false-negative diagnostics */
    [[nodiscard]]
    static std::vector<companion_stub> companion_stubs();
  private:
    std::string m_QualifiedName{};

    template_data m_TemplateData{};

    std::vector<std::string> m_EquivalentTypes{};

    std::filesystem::path m_SourceDir{};

    void transform_file(std::string& text) const;

    void set_header_text(std::string& text, std::string_view copyright, std::string_view nameSpace) const;

    [[nodiscard]]
    std::filesystem::path where_header_absent(const std::filesystem::path& filename) const;

    void generate_header(const std::filesystem::path& headerPath, const std::string& nameSpace);
  };

  class nascent_allocation_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    [[nodiscard]]
    static std::vector<std::string> stubs();

    void finalize();

    [[nodiscard]]
    std::vector<std::string> test_classes() const;
  private:
    void transform_file(std::string& text) const;
  };

  class nascent_behavioural_test : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    void finalize();

    [[nodiscard]]
    std::vector<std::string> test_classes() const;

    [[nodiscard]]
    friend bool operator==(const nascent_behavioural_test&, const nascent_behavioural_test&) noexcept = default;

    [[nodiscard]]
    static std::vector<std::string> stubs();

    void set_namespace(std::string n) { m_Namespace = std::move(n); }
  private:
    void transform_file(std::string& text) const;

    std::string m_Namespace;

    [[nodiscard]]
    std::filesystem::path where_header_absent(const std::filesystem::path& filename) const;

    void generate_header(const std::filesystem::path& headerPath);
  };


  using nascent_test_factory = object::factory<nascent_semantics_test, nascent_allocation_test, nascent_behavioural_test>;
  using nascent_test_vessel = nascent_test_factory::vessel;
}
