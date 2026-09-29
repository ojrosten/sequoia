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
  class failure_reporting_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    /// Serial, since it swaps the terminate handler, which a runner nested in a concurrent test could restore
    using parallelizable_type = std::false_type;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_describe_exception();

    void test_scoped_terminate_handler();
  };

  /** \brief Runs among the parallel tests, on a worker thread, which under MSVC has a terminate handler of its own. */
  class failure_reporting_in_parallel_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  };
}
