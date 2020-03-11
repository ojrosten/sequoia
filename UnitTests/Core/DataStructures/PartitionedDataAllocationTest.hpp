////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PartitionedDataAllocationTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  class partitioned_data_allocation_test final : public regular_allocation_test
  {
  public:
    using regular_allocation_test::regular_allocation_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
    
    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:    
    template<class Test>
    friend void do_allocation_tests(Test&);

    void run_tests() final;

    template<class T, class SharingPolicy, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_bucketed_allocation();

    template<class T, class SharingPolicy, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_contiguous_allocation();
  };
}
