////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class exception_safety_test_utilities_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_steps_counted();

    void test_chosen_step_fails();

    void test_monitor_counts_only_its_lifetime();

    void test_each_thread_monitored_apart();

    void test_nested_monitor_refused();

    void test_fallible_copy();
  };
}
