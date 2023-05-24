////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/MoveOnlyAllocationTestCore.hpp"

#include "sequoia/Core/Object/DataPool.hpp"

namespace sequoia::testing
{
  class data_pool_allocation_test final : public move_only_allocation_test
  {
  public:
    using move_only_allocation_test::move_only_allocation_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    template<bool PropagateMove, bool PropagateSwap>
    void test_allocation();

    void run_tests();
  };
}
