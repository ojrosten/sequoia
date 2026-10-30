////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "PerformanceTestingUtilities.hpp"

namespace sequoia::testing
{
  class performance_false_negative_diagnostics final : public scripted_clock_false_negative_test
  {
  public:
    using scripted_clock_false_negative_test::scripted_clock_false_negative_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_relative_performance();

    void test_confidence_multiplier();

    void test_significance_gate();

    void test_fail_at_second_attempt();

    void test_confidence_multiplier_from_first_attempt();

    void test_relative_performance_with_baselines();

    void test_tasks_no_longer_than_baselines();

    void test_fail_at_second_attempt_with_baselines();
  };

  class performance_false_positive_diagnostics final : public scripted_clock_false_positive_test
  {
  public:
    using scripted_clock_false_positive_test::scripted_clock_false_positive_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_relative_performance();

    void test_confidence_multiplier();

    void test_relative_performance_with_baselines();

    void test_tasks_no_longer_than_baselines();

    void test_baselines_doing_nothing();
  };
}
