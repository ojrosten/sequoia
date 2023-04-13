////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularAllocationTestCore.hpp"

namespace sequoia::testing
{
  class scoped_allocation_false_negative_diagnostics final
    : public regular_allocation_false_negative_test
  {
  public:
    using regular_allocation_false_negative_test::regular_allocation_false_negative_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:
    void run_tests() final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_perfectly_scoped();

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_perfectly_mixed();

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_weirdly_mixed();

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_perfectly_branched();

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_three_level_scoped();
  };

  class scoped_allocation_false_positive_diagnostics final
    : public regular_allocation_false_positive_test
  {
  public:
    using regular_allocation_false_positive_test::regular_allocation_false_positive_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:
    void run_tests() final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_regular_semantics();
  };
}
