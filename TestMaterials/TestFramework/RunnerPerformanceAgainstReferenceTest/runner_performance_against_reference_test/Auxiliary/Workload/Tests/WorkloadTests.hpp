////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace workload
{
  /** The base of tests which do little but leave files behind, for the
      next run's preparation of the test's materials to discard.
   */
  class workload_test_base : public sequoia::testing::free_test
  {
  public:
    using sequoia::testing::free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    /** Writes files to the scratchpad, then makes one check. */
    void run_tests();
  };

  class workload_test_0 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };

  class workload_test_1 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };

  class workload_test_2 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };

  class workload_test_3 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };

  class workload_test_4 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };

  class workload_test_5 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };

  class workload_test_6 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };

  class workload_test_7 final : public workload_test_base
  {
  public:
    using workload_test_base::workload_test_base;
  };
}
