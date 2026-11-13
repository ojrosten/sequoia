////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

#include <string_view>

namespace sequoia::testing
{
  class test_runner_performance_check_outcomes_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_recording_of_performance_check_attempts();

    void test_summary_of_performance_checks_within_other_checks();

    void test_summary_of_failing_performance_checks();

    template<concrete_test... Tests>
    void check_summaries(const reporter& description, std::string_view directory);
  };
}
