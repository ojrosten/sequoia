////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

/** \file */

namespace sequoia::testing
{

  class test_runner_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_exceptions();

    void test_critical_errors();

    void test_filtered_suites();

    void test_basic_output();

    void test_help_output();

    void test_verbose_output();

    void test_serial_verbose_output();

    void test_throwing_tests();

    void test_prune_basic_output();

    void test_prune_with_changed_toolchain();

    void test_prune_selects_a_test_this_executable_lacks();

    struct fake_build
    {
      std::filesystem::path source, toolchainHeader;
    };

    [[nodiscard]]
    fake_build write_fake_build();

    void test_post_run_failure();

    void test_nested_suite();

    void test_nested_suite_verbose();

    void test_suite_named_as_a_sibling_test();

    void test_excluded_performance_tests();

    void test_excluded_tests();

    void test_excluded_tests_are_rerun();

    void test_dump_comparison();

    void test_instability_analysis();

    template<std::invocable<test_runner&> Manipulator, concrete_test... Ts>
    void test_instability_analysis(std::string_view message,
                                   std::string_view outputDirName,
                                   std::string_view numRuns,
                                   return_code expected,
                                   std::initializer_list<std::string_view> extraArgs,
                                   Manipulator manipulator,
                                   Ts&&... ts);

    template<concrete_test... Ts>
    void test_instability_analysis(std::string_view message,
                                   std::string_view outputDirName,
                                   std::string_view numRuns,
                                   return_code expected,
                                   std::initializer_list<std::string_view> extraArgs,
                                   Ts&&... ts);

    template<concrete_test... Ts>
    void test_instability_analysis(std::string_view message,
                                   std::string_view outputDirName,
                                   std::string_view numRuns,
                                   return_code expected,
                                   Ts&&... ts);

    [[nodiscard]]
    std::filesystem::path fake_project() const;

    [[nodiscard]]
    std::filesystem::path minimal_fake_path() const;

    [[nodiscard]]
    std::string zeroth_arg() const;

    void write(std::string_view dirName, std::stringstream& output) const;

    void check_output(reporter description, std::string_view dirName, std::stringstream& output);
  };
}
