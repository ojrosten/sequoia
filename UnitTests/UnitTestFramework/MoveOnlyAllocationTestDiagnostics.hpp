////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "AllocationTestCore.hpp"
#include "UnitTestUtilities.hpp"

namespace sequoia::unit_testing
{
  class move_only_allocation_false_positive_diagnostics final : public move_only_allocation_false_positive_test
  {
  public:    
    using move_only_allocation_false_positive_test::move_only_allocation_false_positive_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    friend move_only_allocation_false_positive_test;
    
    void run_tests() final;

    template<bool PropagateMove, bool PropagateSwap>
    void test_allocation();

    template<bool PropagateMove, bool PropagateSwap>
    void test_move_only_semantics_allocations();
  };

  class move_only_allocation_false_negative_diagnostics final : public move_only_allocation_false_negative_test
  {
  public:
    using move_only_allocation_false_negative_test::move_only_allocation_false_negative_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    friend move_only_allocation_false_negative_test;
    
    void run_tests() final;

    template<bool PropagateMove, bool PropagateSwap>
    void test_allocation();

    template<bool PropagateMove, bool PropagateSwap>
    void test_move_only_semantics_allocations();
  };
}
