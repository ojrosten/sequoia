////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularAllocationTestCore.hpp"

namespace curlew::testing
{
    using namespace sequoia::testing;

    class container_allocation_test final : public regular_allocation_test
    {
    public:
        using regular_allocation_test::regular_allocation_test;

        template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
        void test_allocation();

        [[nodiscard]]
        std::filesystem::path source_file() const;

        void run_tests();
    };
}
