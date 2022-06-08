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

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sequoia::testing
{
  inline constexpr std::string_view seqpat{".seqpat"};

  /*! \brief Paths which are potentially discoverable from the commandline arguments */

  class discoverable_paths
  {
  public:
    discoverable_paths(int argc, char** argv);

    [[nodiscard]]
    const std::filesystem::path& root() const noexcept
    {
      return m_Root;
    }

    [[nodiscard]]
    const std::filesystem::path& executable() const noexcept
    {
      return m_Executable;
    }

    [[nodiscard]]
    friend bool operator==(const discoverable_paths&, const discoverable_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const discoverable_paths&, const discoverable_paths&) noexcept = default;
  private:
    std::filesystem::path m_Root, m_Executable;

    discoverable_paths(std::filesystem::path rt, std::filesystem::path ex);

    [[nodiscard]]
    static discoverable_paths make(int argc, char** argv);
  };

  /*! \brief Paths relating to the `main` cpp */

  class main_paths
  {
  public:
    main_paths(std::filesystem::path file, std::filesystem::path commonIncludes);

    explicit main_paths(std::filesystem::path file);

    [[nodiscard]]
    const std::filesystem::path& file() const noexcept { return m_File; }

    [[nodiscard]]
    const std::filesystem::path& dir() const noexcept { return m_Dir; }

    [[nodiscard]]
    const std::filesystem::path& common_includes() const noexcept { return m_CommonIncludes; }

    [[nodiscard]]
    std::filesystem::path cmake_lists() const;

    [[nodiscard]]
    static std::filesystem::path default_main_cpp_from_root();

    [[nodiscard]]
    static std::filesystem::path default_cmake_from_root();

    [[nodiscard]]
    friend bool operator==(const main_paths&, const main_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const main_paths&, const main_paths&) noexcept = default;
  private:
    std::filesystem::path m_File{}, m_Dir{}, m_CommonIncludes{};
  };

  /*! \brief Paths relating to the source directory */

  class source_paths
  {
  public:
    explicit source_paths(const std::filesystem::path& projectRoot);

    [[nodiscard]]
    const std::filesystem::path& project() const noexcept
    {
      return m_Project;
    }

    [[nodiscard]]
    const std::filesystem::path& source_root() const noexcept
    {
      return m_SourceRoot;
    }

    [[nodiscard]]
    std::filesystem::path cmake_lists() const;

    [[nodiscard]]
    static std::filesystem::path cmake_lists(std::filesystem::path projectRoot);

    [[nodiscard]]
    friend bool operator==(const source_paths&, const source_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const source_paths&, const source_paths&) noexcept = default;
  private:
  private:
    std::filesystem::path m_SourceRoot, m_Project;

    [[nodiscard]]
    static std::filesystem::path source_root(std::filesystem::path projectRoot);
  };

  /*! \brief Paths relating to the build directory */

  class build_paths
  {
  public:
    explicit build_paths(std::filesystem::path projectRoot, const main_paths& main);

    [[nodiscard]]
    const std::filesystem::path& dir() const { return m_Dir; }

    [[nodiscard]]
    const std::filesystem::path& cmade_dir() const noexcept
    {
      return m_CMadeBuildDir;
    }

    [[nodiscard]]
    std::filesystem::path cmake_cache() const;

    [[nodiscard]]
    friend bool operator==(const build_paths&, const build_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const build_paths&, const build_paths&) noexcept = default;
  private:
    std::filesystem::path m_Dir, m_CMadeBuildDir{};

    [[nodiscard]]
    std::filesystem::path cmade_dir(const main_paths& main);
  };

  /*! \brief Paths for auxiliary materials, used in creating projects/tests */

  class auxiliary_paths
  {
  public:
    auxiliary_paths(const std::filesystem::path& projectRoot);

    [[nodiscard]]
    const std::filesystem::path& dir() const noexcept
    {
      return m_Dir;
    }

    [[nodiscard]]
    static std::filesystem::path dir(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& test_templates() const noexcept
    {
      return m_TestTemplates;
    }

    [[nodiscard]]
    static std::filesystem::path test_templates(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& source_templates() const noexcept
    {
      return m_SourceTemplates;
    }

    [[nodiscard]]
    static std::filesystem::path source_templates(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& project_template() const noexcept
    {
      return m_ProjectTemplate;
    }

    [[nodiscard]]
    static std::filesystem::path project_template(std::filesystem::path projectRoot);

    [[nodiscard]]
    friend bool operator==(const auxiliary_paths&, const auxiliary_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const auxiliary_paths&, const auxiliary_paths&) noexcept = default;
  private:
    std::filesystem::path
      m_Dir{},
      m_TestTemplates{},
      m_SourceTemplates{},
      m_ProjectTemplate{};
  };

  /*! \brief Holds details of the file to which the last successfully completed test is registered.

    If a check causes a crash, the recovery file may be used to provide a clue as to where this
    happened.
 */
  class recovery_paths
  {
  public:
    recovery_paths(const std::filesystem::path& outputDir);

    [[nodiscard]]
    const std::filesystem::path& dir() const noexcept
    {
      return m_Dir;
    }

    [[nodiscard]]
    static std::filesystem::path dir(std::filesystem::path outputDir);

    [[nodiscard]]
    std::filesystem::path recovery_file() const;

    [[nodiscard]]
    std::filesystem::path dump_file() const;

    [[nodiscard]]
    friend bool operator==(const recovery_paths&, const recovery_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const recovery_paths&, const recovery_paths&) noexcept = default;
  private:
    std::filesystem::path m_Dir{};
  };

  /*! \brief Paths used when using dependencies to prune the number of tests */

  class prune_paths
  {
  public:
    prune_paths(std::filesystem::path outputDir, const std::filesystem::path& buildRoot, const std::filesystem::path& buildDir);

    [[nodiscard]]
    const std::filesystem::path& dir() const noexcept
    {
      return m_Dir;
    }

    [[nodiscard]]
    std::filesystem::path stamp() const;

    [[nodiscard]]
    std::filesystem::path failures(std::optional<std::size_t> id) const;

    [[nodiscard]]
    std::filesystem::path selected_passes(std::optional<std::size_t> id) const;

    [[nodiscard]]
    std::filesystem::path external_dependencies() const;

    [[nodiscard]]
    std::filesystem::path instability_analysis() const;

    [[nodiscard]]
    friend bool operator==(const prune_paths&, const prune_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const prune_paths&, const prune_paths&) noexcept = default;
  private:
    std::filesystem::path m_Dir{}, m_Stem{};

    [[nodiscard]]
    static std::filesystem::path make_stem(const std::filesystem::path& buildDir);

    [[nodiscard]]
    std::filesystem::path make_path(std::optional<std::size_t> id, std::string_view extension) const;
  };


  /*! \brief Paths in the output directory */

  class output_paths
  {
  public:
    output_paths(const std::filesystem::path& projectRoot);

    [[nodiscard]]
    const std::filesystem::path& dir() const noexcept
    {
      return m_Dir;
    }

    [[nodiscard]]
    static std::filesystem::path dir(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path tests_temporary_data() const noexcept
    {
      return m_TestsTemporaryData;
    }

    [[nodiscard]]
    static std::filesystem::path tests_temporary_data(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& diagnostics() const noexcept
    {
      return m_Diagnostics;
    }

    [[nodiscard]]
    static std::filesystem::path diagnostics(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& test_summaries() const noexcept
    {
      return m_TestSummaries;
    }

    [[nodiscard]]
    static std::filesystem::path test_summaries(std::filesystem::path projectRoot);

    [[nodiscard]]
    const std::filesystem::path& instability_analysis() const noexcept
    {
      return m_InstabilityAnalysis;
    }

    [[nodiscard]]
    static std::filesystem::path instability_analysis(std::filesystem::path projectRoot);

    [[nodiscard]]
    recovery_paths recovery() const
    {
      return {dir()};
    }

    [[nodiscard]]
    prune_paths prune(const std::filesystem::path& buildRoot, const std::filesystem::path& buildDir) const
    {
      return {dir(), buildRoot, buildDir};
    }

    [[nodiscard]]
    friend bool operator==(const output_paths&, const output_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const output_paths&, const output_paths&) noexcept = default;
  private:
    std::filesystem::path
      m_Dir{},
      m_TestsTemporaryData{},
      m_Diagnostics{},
      m_TestSummaries{},
      m_InstabilityAnalysis{};
  };

  /*! \brief Paths used by the project */

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
      return m_Discovered.root();
    }

    [[nodiscard]]
    const std::filesystem::path& executable() const noexcept
    {
      return m_Discovered.executable();
    }

    [[nodiscard]]
    const source_paths& source() const noexcept
    {
      return m_Source;
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
    const build_paths& build() const noexcept
    {
      return m_Build;
    }

    [[nodiscard]]
    const std::filesystem::path& build_system() const noexcept
    {
      return m_BuildSystem;
    }

    [[nodiscard]]
    static std::filesystem::path build_system(std::filesystem::path projectRoot);

    [[nodiscard]]
    const auxiliary_paths& aux_paths() const noexcept
    {
      return m_Auxiliary;
    }

    [[nodiscard]]
    const output_paths& output() const noexcept
    {
      return m_Output;
    }

    [[nodiscard]]
    const main_paths& main() const noexcept
    {
      return m_Main;
    }

    [[nodiscard]]
    const std::vector<main_paths>& ancillary_main_cpps() const noexcept
    {
      return m_AncillaryMainCpps;
    }

    [[nodiscard]]
    prune_paths prune() const;

    [[nodiscard]]
    friend bool operator==(const project_paths&, const project_paths&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const project_paths&, const project_paths&) noexcept = default;
  private:
    discoverable_paths m_Discovered;
    main_paths         m_Main;
    source_paths       m_Source;
    build_paths        m_Build;
    auxiliary_paths    m_Auxiliary;
    output_paths       m_Output;

    std::filesystem::path
      m_Tests{},
      m_TestMaterials{},
      m_BuildSystem{};

    std::vector<main_paths> m_AncillaryMainCpps{};
  };
}
