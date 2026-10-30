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
  /** A performance test of the runner against a reference tree's runner.

      The test builds one project of tests which do little twice: against
      this tree's sequoia and against the reference tree's. The test fails if
      the runner built from this tree, the candidate, takes distinguishably
      longer than a fixed multiple of the reference's time to run the tests.

      In the build that runs the test, the cache variable
      `SEQUOIA_PERFORMANCE_REFERENCE_ROOT` names the reference tree's root.
   */
  class runner_performance_against_reference_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  };
}
