////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"

namespace sequoia::testing
{
  class test_runner;

  class test_runner_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_discriminator_hooks();

    template<template<class> class Probe>
    void test_discriminator_probe();

    void test_exceptions();

    void test_critical_errors();

    void test_filtered_suites();

    void test_suites_not_found();

    void test_selections_not_found();

    void test_basic_output();

    void test_tests_registered_between_executions();

    void test_execution_after_an_execution_which_threw();

    void test_help_output();

    void test_verbose_output();

    void test_serial_verbose_output();

    void test_throwing_tests();

    void test_execution_records();

    void test_discriminated_summary();

    void test_prune_basic_output();

    void test_prune_with_changed_toolchain();

    void test_prune_selects_a_test_this_executable_lacks();

    void test_prune_with_nothing_stale();

    struct fake_build
    {
      std::filesystem::path source, toolchainHeader;
    };

    [[nodiscard]]
    fake_build write_fake_build();

    void test_post_run_failure();

    void test_materials_update();

    void test_no_materials_update_after_critical_failure();

    void test_partial_materials_update();

    void test_discriminated_materials_update();

    void test_materials_update_of_two_tests();

    void test_materials_preparation_failure();

    void test_versioned_output_failure();

    void test_versioned_output_check();

    void test_discarded_materials_removal();

    void test_discarded_materials_removal_failure();

    void test_discarded_materials_removal_exception();

    void test_nested_suite();

    void test_nested_suite_verbose();

    void test_suite_named_as_a_sibling_test();

    void test_excluded_performance_tests();

    void test_excluded_tests();

    void test_excluded_tests_are_rerun();

    void test_dump_comparison();

    void test_thread_pool();

    void test_instability_analysis();

    void test_instability_analysis_in_sandboxes_from_a_path_with_a_space();

    void test_sandboxed_repetition();

    void test_exit_statuses();

    void test_return_code_names();

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
