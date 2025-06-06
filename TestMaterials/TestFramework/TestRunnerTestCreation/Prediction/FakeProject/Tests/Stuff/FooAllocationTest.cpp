////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FooAllocationTest.hpp"

namespace fakeProject::testing
{
    [[nodiscard]]
    std::filesystem::path foo_allocation_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void foo_allocation_test::run_tests()
    {
        do_allocation_tests();
    }

    template<bool PropagateMove, bool PropagateSwap>
    void foo_allocation_test::test_allocation()
    {
        // See e.g RegularAllocationTestDiagnostics.cpp or MoveOnlyAllocationTestDiagostics.cpp
    }
}
