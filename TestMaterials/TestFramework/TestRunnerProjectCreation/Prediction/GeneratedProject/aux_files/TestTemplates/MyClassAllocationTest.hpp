////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "?AllocationTestCore.hpp"

namespace sequoia::testing
{
  class ?_class_allocation_test final : public ?_allocation_test
  {
  public:
    using ?_allocation_test::?_allocation_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:
    void run_tests() final;
  };
}
