////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief File paths and related utilities.
 */

#include "sequoia/TestFramework/CoreInfrastructure.hpp"

#include <filesystem>

namespace sequoia::testing
{
  inline constexpr std::string_view seqpat{".seqpat"};
  const static auto working_path_v{std::filesystem::current_path().lexically_normal()};

  template<>
  struct serializer<std::filesystem::path>
  {
    [[nodiscard]]
    static std::string make(const std::filesystem::path& p);
  };

  template<>
  struct serializer<std::filesystem::file_type>
  {
    [[nodiscard]]
    static std::string make(const std::filesystem::file_type& val);
  };

  [[nodiscard]]
  std::filesystem::path working_path();

  class project_paths
  {
  public:
    explicit project_paths(const std::filesystem::path& projectRoot);

    project_paths(const std::filesystem::path& projectRoot, std::filesystem::path mainCpp, std::filesystem::path includePath);

    [[nodiscard]]
    static std::filesystem::path source_path(const std::filesystem::path& projectRoot);

    [[nodiscard]]
    const std::filesystem::path& project_root() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& source() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& source_root() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& tests() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& test_materials() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& output() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& main_cpp() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& main_cpp_dir() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& include_target() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& cmade_build_dir() const noexcept;

    [[nodiscard]]
    friend bool operator==(const project_paths&, const project_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const project_paths&, const project_paths&) noexcept = default;

    [[nodiscard]]
    static std::filesystem::path cmade_build_dir(const std::filesystem::path& projectRoot, const std::filesystem::path& mainCppDir);
  private:
    std::filesystem::path
      m_ProjectRoot{},
      m_Source{},
      m_SourceRoot{},
      m_Tests{},
      m_TestMaterials{},
      m_Output{},
      m_MainCpp{},
      m_MainCppDir{},
      m_IncludeTarget{},
      m_CMadeBuildDir{};
  };


  [[nodiscard]]
  std::filesystem::path project_root(int argc, char** argv, const std::filesystem::path& fallback=working_path().parent_path());

  [[nodiscard]]
  std::filesystem::path aux_files_path(std::filesystem::path projectRoot);

  [[nodiscard]]
  std::filesystem::path build_system_path(std::filesystem::path projectRoot);

  [[nodiscard]]
  std::filesystem::path code_templates_path(std::filesystem::path projectRoot);

  [[nodiscard]]
  std::filesystem::path source_templates_path(std::filesystem::path projectRoot);

  [[nodiscard]]
  std::filesystem::path project_template_path(std::filesystem::path projectRoot);

  [[nodiscard]]
  std::filesystem::path recovery_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path tests_temporary_data_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path diagnostics_output_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path test_summaries_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path temp_test_summaries_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path prune_path(std::filesystem::path outputDir, const std::filesystem::path& testMainDir);

  template<class Pred>
    requires std::invocable<Pred, std::filesystem::path>
  void throw_if(const std::filesystem::path& p, std::string_view message, Pred pred)
  {
    if(pred(p))
    {
      throw std::runtime_error{p.empty() ? std::string{message} : p.generic_string().append(" ").append(message)};
    }
  }

  void throw_unless_exists(const std::filesystem::path& p, std::string_view message="");

  void throw_unless_directory(const std::filesystem::path& p, std::string_view message="");

  void throw_unless_regular_file(const std::filesystem::path& p, std::string_view message="");

  [[nodiscard]]
  std::filesystem::path find_in_tree(const std::filesystem::path& root, const std::filesystem::path& toFind);
  
  [[nodiscard]]
  std::filesystem::path rebase_from(const std::filesystem::path& filename, const std::filesystem::path& dir);
}
