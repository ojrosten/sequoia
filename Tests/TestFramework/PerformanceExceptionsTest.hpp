////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class performance_exceptions_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_invalid_arguments();

    void test_throwing_task();

    void test_non_positive_durations();
  };
}
