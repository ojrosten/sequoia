////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RelationalTestCore.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"

namespace sequoia::testing
{
  struct cmd_builder
  {
    cmd_builder(const std::filesystem::path& projRoot);

    void create_build_run(const std::filesystem::path& creationOutput, std::string_view buildOutput, const std::filesystem::path& output) const;

    void rebuild_run(const std::filesystem::path& outputDir, std::string_view cmakeOutput, std::string_view buildOutput, std::string_view options) const;

    void run_executable(const std::filesystem::path& outputDir, std::string_view options) const;

    std::filesystem::path mainDir, buildDir;

  private:
    [[nodiscard]]
    shell_command run(const std::filesystem::path& outputDir, std::string_view options) const;
  };

  class test_runner_end_to_end_test final : public relational_test
  {
  public:
    using relational_test::relational_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_project_creation();

    [[nodiscard]]
    std::filesystem::path generated_project() const;

    void copy_aux_materials(const std::filesystem::path& relativeFrom, const std::filesystem::path& relativeTo) const;

    void create_run_and_check(std::string_view description, const cmd_builder& b);

    void run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view options);

    void rebuild_run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view CMakeOutput, std::string_view BuildOutput, std::string_view options);

    void check_timings(std::string_view description, const std::filesystem::path& relOutputFile, const int speedupFactor);
  };
}
