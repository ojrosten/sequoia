////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/Commands.hpp"

namespace sequoia::testing
{
  class cmd_builder
  {
  public:
    cmd_builder(const std::filesystem::path& projRoot, const build_paths& applicationBuildPaths);

    void create_build_run(const std::filesystem::path& creationOutput, std::string_view buildOutput, const std::filesystem::path& output) const;

    void rebuild_run(const std::filesystem::path& outputDir, std::string_view cmakeOutput, std::string_view buildOutput, std::string_view options) const;

    void run_executable(const std::filesystem::path& outputDir, std::string_view options) const;

    [[nodiscard]]
    const std::filesystem::path& cmake_cache_dir() const;

    [[nodiscard]]
    const main_paths& get_main_paths() const noexcept { return m_Main; }

    [[nodiscard]]
    const build_paths& get_build_paths() const noexcept { return m_Build; }
  private:
    main_paths m_Main;
    build_paths m_Build;
  };

  class test_runner_end_to_end_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    [[nodiscard]]
    std::string summary_discriminator() const
    {
      return with_msvc_v ? "msvc" : std::string{};
    }

    void run_tests();
  private:
    void test_project_creation();

    [[nodiscard]]
    std::filesystem::path generated_project() const;

    void copy_aux_materials(const std::filesystem::path& relativeFrom, const std::filesystem::path& relativeTo) const;

    void create_run_and_check(std::string_view description, const cmd_builder& b);

    void run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view options);

    void rebuild_run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view CMakeOutput, std::string_view BuildOutput, std::string_view options);

    void check_project_files(std::string_view description, const cmd_builder& b);
  };
}
