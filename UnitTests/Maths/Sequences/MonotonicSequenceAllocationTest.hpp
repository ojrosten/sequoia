////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "AllocationTestCore.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  class monotonic_sequence_allocation_test final : public allocation_test
  {
  public:
    using allocation_test::allocation_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:
    void run_tests() final;
  };
}
