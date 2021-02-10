////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "move_only_allocationAllocationTestCore.hpp"

namespace sequoia::testing
{
  class foo_allocation_test final : public move_only_allocation_allocation_test
  {
  public:
    using move_only_allocation_allocation_test::move_only_allocation_allocation_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:
    void run_tests() final;
  };
}
