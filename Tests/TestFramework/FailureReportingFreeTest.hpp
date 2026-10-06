////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
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
  class failure_reporting_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    /** Serial, since it swaps the terminate handler and the error mode, which a runner nested in a concurrent test
        could restore
     */
    using parallelizable_type = std::false_type;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_describe_exception();

    void test_scoped_terminate_handler();

    void test_windows_crash_report_enabler();
  };

  /** \brief Runs among the parallel tests, on a worker thread, which under MSVC has a terminate handler of its own,
             to check what the runner sets for the run.
   */
  class failure_reporting_in_parallel_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  };
}
