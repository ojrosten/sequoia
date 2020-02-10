////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "AllocationTestCore.hpp"

namespace sequoia::unit_testing
{
  class allocation_false_positive_diagnostics final : public allocation_false_positive_test
  {
  public:
    using allocation_false_positive_test::allocation_false_positive_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    friend allocation_false_positive_test;
    
    void run_tests() final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_regular_semantics();
  };

  class allocation_false_negative_diagnostics final : public allocation_false_negative_test
  {
  public:
    using allocation_false_negative_test::allocation_false_negative_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    friend allocation_false_negative_test;
    
    void run_tests() final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_regular_semantics();
  };
}
