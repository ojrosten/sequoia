////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularAllocationTestCore.hpp"

namespace sequoia::testing
{
  class scoped_allocation_false_negative_diagnostics final
    : public regular_allocation_false_negative_test
  {
  public:
    using regular_allocation_false_negative_test::regular_allocation_false_negative_test; 

    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:
    void run_tests() final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_perfectly_scoped();

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_perfectly_mixed();
  };

  class scoped_allocation_false_positive_diagnostics final
    : public regular_allocation_false_positive_test
  {
  public:
    using regular_allocation_false_positive_test::regular_allocation_false_positive_test; 

    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private: 
    void run_tests() final;
    
    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_regular_semantics();
  };
}
