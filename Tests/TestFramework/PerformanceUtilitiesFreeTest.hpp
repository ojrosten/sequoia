////////////////////////////////////////////////////////////////////
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
  class performance_utilities_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_task_constraints();

    void test_clock_constraints();

    void test_timing_constraints();

    void test_exception_specifications();

    void test_default_minimum_trials();

    void test_overlap_multipliers();

    void test_multiplier_given_to_each_attempt();

    void test_judgement_with_baselines();

    void test_shuffled_baselined_task_orders();

    void test_call_in_order();

    void test_postprocessing();

    void test_zeroed_measurements();
  };
}
