////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ContainerAllocationTest.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view container_allocation_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void container_allocation_test::run_tests()
    {
        do_allocation_tests(*this);
    }

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void container_allocation_test::test_allocation()
    {
        // See e.g RegularAllocationTestDiagnostics.cpp or MoveOnlyAllocationTestDiagostics.cpp
    }
}
