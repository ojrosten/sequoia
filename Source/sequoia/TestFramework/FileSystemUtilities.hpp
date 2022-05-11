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
#include <optional>

namespace sequoia::testing
{
  inline constexpr std::string_view seqpat{".seqpat"};

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

  class file_info
  {
  public:
    file_info(std::filesystem::path file);

    [[nodiscard]]
    const std::filesystem::path& file() const noexcept { return m_File; }

    [[nodiscard]]
    const std::filesystem::path& dir() const noexcept { return m_Dir; }

    [[nodiscard]]
    friend bool operator==(const file_info&, const file_info&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const file_info&, const file_info&) noexcept = default;
  private:
    std::filesystem::path m_File{}, m_Dir{};
  };

  struct discoverable_paths
  {
    std::filesystem::path root, executable;
  };

  class project_paths
  {
  public:
    struct initializer
    {
      std::filesystem::path mainCpp;

      std::vector<std::string> ancillaryMainCpps{};

      std::filesystem::path commonIncludes{};
    };

    project_paths(int argc, char** argv, const initializer& pathsFromRoot);

    [[nodiscard]]
    const std::filesystem::path& project_root() const noexcept
    {
      return m_Discovered.root;
    }

    [[nodiscard]]
    const std::filesystem::path& executable() const noexcept
    {
      return m_Discovered.executable;
    }

    [[nodiscard]]
    const std::filesystem::path& source() const noexcept
    {
      return m_Source;
    }

    [[nodiscard]]
    static std::filesystem::path source(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& source_root() const noexcept
    {
      return m_SourceRoot;
    }

    [[nodiscard]]
    const std::filesystem::path& tests() const noexcept
    {
      return m_Tests;
    }

    [[nodiscard]]
    static std::filesystem::path tests(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& test_materials() const noexcept
    {
      return m_TestMaterials;
    }

    [[nodiscard]]
    static std::filesystem::path test_materials(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& output() const noexcept
    {
      return m_Output;
    }

    [[nodiscard]]
    static std::filesystem::path output(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& build() const noexcept
    {
      return m_Build;
    }

    [[nodiscard]]
    static std::filesystem::path build(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& aux_files() const noexcept
    {
      return m_AuxFiles;
    }

    [[nodiscard]]
    static std::filesystem::path aux_files(std::filesystem::path projectRoot);

    [[nodiscard]]
    const file_info& main_cpp() const noexcept
    {
      return m_MainCpp;
    }

    [[nodiscard]]
    const std::filesystem::path& common_includes() const noexcept
    {
      return m_CommonIncludes;
    }

    [[nodiscard]]
    const std::filesystem::path& cmade_build_dir() const noexcept
    {
      return m_CMadeBuildDir;
    }

    [[nodiscard]]
    const std::filesystem::path& prune_dir() const noexcept
    {
      return m_PruneDir;
    }

    [[nodiscard]]
    const std::filesystem::path& instability_analysis_prune_dir() const noexcept
    {
      return m_InstabilityAnalysisPruneDir;
    }

    [[nodiscard]]
    const std::vector<file_info>& ancillary_main_cpps() const noexcept
    {
      return m_AncillaryMainCpps;
    }

    [[nodiscard]]
    std::filesystem::path prune_file_path(std::optional<std::size_t> id) const;

    [[nodiscard]]
    friend bool operator==(const project_paths&, const project_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const project_paths&, const project_paths&) noexcept = default;

    [[nodiscard]]
    static std::filesystem::path cmade_build_dir(const std::filesystem::path& projectRoot, const std::filesystem::path& mainCppDir);
  private:
    discoverable_paths m_Discovered;

    file_info m_MainCpp;
    
    std::filesystem::path
      m_Source{},
      m_SourceRoot{},
      m_Tests{},
      m_TestMaterials{},
      m_Output{},
      m_Build{},
      m_AuxFiles{},
      m_CommonIncludes{},
      m_CMadeBuildDir{},
      m_PruneDir{},
      m_InstabilityAnalysisPruneDir{};

    std::vector<file_info> m_AncillaryMainCpps{};
  };


  [[nodiscard]]
  discoverable_paths discover_paths(int argc, char** argv);

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

  template<std::predicate<std::filesystem::path> Pred>
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
