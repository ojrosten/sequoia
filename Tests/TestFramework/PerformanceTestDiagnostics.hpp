//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class performance_false_negative_diagnostics final : public performance_false_negative_test
  {
  public:
    using performance_false_negative_test::performance_false_negative_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_relative_performance();

    void test_sd_tolerance();

    void test_significance_gate();
  };

  class performance_false_positive_diagnostics final : public performance_false_positive_test
  {
  public:
    using performance_false_positive_test::performance_false_positive_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_relative_performance();

    void test_sd_tolerance();
  };

  class performance_utilities_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_postprocessing();

    void test_coarse_sleep();

    void test_invalid_arguments();

    void test_throwing_task();

    void test_task_constraints();
  };

  class performance_retry_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_stop_at_first_passing_attempt();

    void test_failure_once_attempts_run_out();

    void test_false_negative_stop_at_first_failing_attempt();
  };
}
