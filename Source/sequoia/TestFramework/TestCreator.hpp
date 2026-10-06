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
  /** \brief Whether to append `&` to `spelling`, which spells a parameter's
             type.

      The function reads only the text of `spelling`; it looks up no type.

      \throws std::logic_error if `ascii::is_empty_or_whitespace(spelling)`.
   */
  [[nodiscard]]
  bool needs_reference_suffix(std::string_view spelling);

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

  /** \brief Produces the name of the directory `sourceProject`, so long as it can be used as an identifier.

      \throws std::runtime_error if `sourceProject` is not a directory, or its name is not an identifier.
   */
  [[nodiscard]]
  std::string project_namespace_for(const std::filesystem::path& sourceProject);

  enum class add_to_common_includes { no, yes };

  /** \brief The specification of a companion file: its stub, and whether to add the file to the common
      includes.

      `stub` ends both the companion file's name and the name of the file in the test templates directory
      from which the companion file is created. `include` is ignored unless the companion file is a header.
   */
  struct companion_specification
  {
    std::string stub{};
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

    /** \brief The test's full name, if one was given */
    [[nodiscard]]
    const std::optional<std::string>& full_name() const noexcept { return m_FullName; }

    void full_name(std::string name) { m_FullName = std::move(name); }

    /** \brief The path of the testing utilities, or an empty path if none was given */
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

    /** \brief Creates the files, then registers the test classes such that they are executed.

        Each companion file is named `type_file_stem()` followed by the `stub` of its specification. Each
        of the test's own files is named `test_file_stem()` followed by the extension of its stub. A file
        which already exists is not overwritten.

        `generatedHeaderPath` is called with the name of the header under test, and `generate` with the path
        `generatedHeaderPath` returns. Neither is called unless the header cannot be found and its generation was
        requested. If its generation was requested but the header is found, a warning names the header found, which
        may lie outside the directory requested.

        \throws std::runtime_error if
        -# The header under test cannot be found and is not to be generated;
        -# `testing_utilities()` does not name exactly one regular file anywhere within the tests repository;
        -# A full name was given which cannot name the test.

        These conditions are checked before any file is written.
     */
    template<invocable_exact_r<std::filesystem::path, std::filesystem::path> GeneratedHeaderPath,
             std::invocable<std::filesystem::path> Generator,
             std::invocable<std::string&> FileTransformer>
    void finalize(GeneratedHeaderPath generatedHeaderPath,
                  Generator generate,
                  const std::vector<companion_specification>& companionSpecifications,
                  const std::vector<std::string>& ownStubs,
                  const std::vector<std::string>& testClasses,
                  std::string_view nameStub,
                  FileTransformer transformer);

    /** \brief The full name if one was given, else `name_from_forename_and_surname()` */
    [[nodiscard]]
    std::string test_name() const;

    /** \brief `<forename>_<surname>` */
    [[nodiscard]]
    std::string name_from_forename_and_surname() const;

    /** \brief `test_name()` in camel case */
    [[nodiscard]]
    std::string test_file_stem() const;

    /** \brief The generic form of the path of the testing utilities: relative to `host_dir()` if they lie
        within `host_dir()`, and otherwise relative to the tests repository.
     */
    [[nodiscard]]
    std::string testing_utilities_include() const;

    [[nodiscard]]
    const std::string& type_file_stem() const noexcept { return m_TypeFileStem; }

    /** \brief Sets `type_file_stem()` to `typeName` in camel case.

        If `header()` is empty, also sets `header()` to `type_file_stem()` followed by `.hpp`.
     */
    void name_files_after_type(std::string_view typeName);

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

    /** \brief Replaces `testing_utilities()` with the path of the regular file it names anywhere within the
        tests repository.

        \throws std::runtime_error if `testing_utilities()` names no such file, or several.
     */
    void locate_testing_utilities();

    /** \brief Checks that the full name can name the test.

        \throws std::runtime_error if the full name
        -# Is not an identifier;
        -# Names a test which is already registered;
        -# Would give one of the test's own files the name of a companion file, or of an entry already
           in the file's directory, ignoring case.
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

  /** \brief The base of a test of a class whose header may be generated.

      `source_dir` sets the directory, within the project's source, in which a header named by a relative path
      is generated.
   */
  class nascent_class_test_base : public nascent_test_base
  {
  public:
    using nascent_test_base::nascent_test_base;

    void source_dir(std::filesystem::path dir) { m_SourceDir = std::move(dir); }

    [[nodiscard]]
    friend bool operator==(const nascent_class_test_base&, const nascent_class_test_base&) noexcept = default;
  protected:
    nascent_class_test_base(const nascent_class_test_base&)     = default;
    nascent_class_test_base(nascent_class_test_base&&) noexcept = default;
    nascent_class_test_base& operator=(const nascent_class_test_base&)     = default;
    nascent_class_test_base& operator=(nascent_class_test_base&&) noexcept = default;

    ~nascent_class_test_base() = default;

    [[nodiscard]]
    std::filesystem::path generated_header_path(const std::filesystem::path& filename) const;

    /** \brief Generates a header declaring the class `forename()` in `nameSpace`, and a source file if
        `templateData` is empty.

        The class is a template if `templateData` is not empty, and is regular or move-only as `semantics` is
        `regular` or `move_only`. The path of each file generated is printed.
     */
    void generate_header(const std::filesystem::path& headerPath,
                         std::string_view semantics,
                         std::string_view nameSpace,
                         const template_data& templateData);
  private:
    std::filesystem::path m_SourceDir{};

    void set_header_text(std::string& text, std::string_view nameSpace, const template_data& templateData) const;
  };

  class nascent_semantics_test : public nascent_class_test_base
  {
  public:
    using nascent_class_test_base::nascent_class_test_base;

    void qualified_name(std::string name) { m_QualifiedName = std::move(name); }

    void equivalent_type(std::string name) { m_EquivalentType = std::move(name); }

    void finalize();

    [[nodiscard]]
    std::vector<std::string> test_classes() const;

    [[nodiscard]]
    friend bool operator==(const nascent_semantics_test&, const nascent_semantics_test&) noexcept = default;

    /** \brief The stubs of the test's own files */
    [[nodiscard]]
    static std::vector<std::string> stubs();

    /** \brief The specifications of the companion files: the testing utilities, and the header and source
        of the false-negative diagnostics.
     */
    [[nodiscard]]
    static std::vector<companion_specification> companion_specifications();
  private:
    std::string m_QualifiedName{};

    template_data m_TemplateData{};

    std::string m_EquivalentType{};

    void transform_file(std::string& text) const;
  };

  class nascent_allocation_test : public nascent_class_test_base
  {
  public:
    using nascent_class_test_base::nascent_class_test_base;

    [[nodiscard]]
    static std::vector<std::string> stubs();

    void finalize();

    [[nodiscard]]
    std::vector<std::string> test_classes() const;

    [[nodiscard]]
    friend bool operator==(const nascent_allocation_test&, const nascent_allocation_test&) noexcept = default;
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
    std::filesystem::path generated_header_path(const std::filesystem::path& filename) const;

    void generate_header(const std::filesystem::path& headerPath);
  };


  using nascent_test_factory = object::factory<nascent_semantics_test, nascent_allocation_test, nascent_behavioural_test>;
  using nascent_test_vessel = nascent_test_factory::vessel;
}
