////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for creating new tests, especially from the commandline.
  */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include "sequoia/Core/Object/Factory.hpp"
#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <array>
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

  void cmake_nascent_tests(const project_paths& projPaths, std::ostream& stream);

  enum class nascent_test_flavour { standard, framework_diagnostics };

  class nascent_test_base
  {
  public:
    enum class gen_source_option {no, yes};

    nascent_test_base(project_paths paths, std::string copyright, indentation codeIndent, std::ostream& stream)
      : m_Paths{std::move(paths)}
      , m_Copyright{std::move(copyright)}
      , m_CodeIndent{codeIndent}
      , m_Stream{&stream}
      , m_ProjectNamespace{back(m_Paths.source().project()).string()}
    {}

    [[nodiscard]]
    nascent_test_flavour flavour() const noexcept { return m_Flavour; }

    void flavour(nascent_test_flavour f) { m_Flavour = f; }

    [[nodiscard]]
    const std::string& suite() const noexcept { return m_Suite; }

    void suite(std::string name) { m_Suite = std::move(name); }

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

    template<invocable_r<std::filesystem::path, std::filesystem::path> WhenAbsent,std::invocable<std::string&> FileTransformer>
    void finalize(WhenAbsent fn,
                  const std::vector<std::string>& stubs,
                  const std::vector<std::string>& constructors,
                  std::string_view nameStub,
                  FileTransformer transformer);

    [[nodiscard]]
    const std::string& camel_name() const noexcept { return m_CamelName; }

    void camel_name(std::string name);

    void set_cpp(const std::filesystem::path& headerPath, std::string_view copyright, std::string_view nameSpace);

    [[nodiscard]]
    const indentation& code_indent() const noexcept { return m_CodeIndent; }

    [[nodiscard]]
    const std::string& copyright() const noexcept { return m_Copyright; }

    [[nodiscard]]
    const std::string& project_namespace() const noexcept { return m_ProjectNamespace; }

    [[nodiscard]]
    std::ostream& stream() noexcept { return *m_Stream; }

    void finalize_suite(std::string_view fallbackIngredient);

    void make_common_replacements(std::string& text) const;
  private:
    constexpr static std::array<std::string_view, 3> st_HeaderExtensions{".hpp", ".h", ".hxx"};

    project_paths m_Paths;
    std::string m_Copyright{};
    indentation m_CodeIndent{"  "};
    std::ostream* m_Stream;

    nascent_test_flavour m_Flavour{nascent_test_flavour::standard};
    std::string 
      m_Suite{},
      m_TestType{},
      m_Forename{},
      m_Surname{},
      m_CamelName{},
      m_ProjectNamespace{};
    std::filesystem::path m_Header{}, m_HostDir{}, m_HeaderPath{};
    gen_source_option m_SourceOption{};

    void on_source_path_error() const;

    void finalize_header(const std::filesystem::path& sourcePath);

    template<std::invocable<std::string&> FileTransformer>
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
    static std::vector<std::string> stubs();
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

    [[nodiscard]]
    static std::vector<std::string> stubs();

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
    static std::vector<std::string> stubs();

    void set_namespace(std::string n) { m_Namespace = std::move(n); }
  private:
    void transform_file(std::string& text) const;

    std::string m_Namespace;

    [[nodiscard]]
    std::filesystem::path when_header_absent(const std::filesystem::path& filename);
   };


  using nascent_test_factory = object::factory<nascent_semantics_test, nascent_allocation_test, nascent_behavioural_test>;
  using nascent_test_vessel = typename nascent_test_factory::vessel;
}
