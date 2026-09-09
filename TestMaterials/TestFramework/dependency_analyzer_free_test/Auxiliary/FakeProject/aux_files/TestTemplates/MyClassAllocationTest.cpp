////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "?ClassAllocationTest.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path ?_class_allocation_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void ?_class_allocation_test::run_tests()
	{
		do_allocation_tests(*this);
	}

	template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
	void ?_class_allocation_test::test_allocation()
	{
		// See e.g RegularAllocationTestDiagnostics.cpp or MoveOnlyAllocationTestDiagostics.cpp
	}
}
