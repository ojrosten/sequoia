////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "?TestFile.hpp"

namespace ?::testing
{
	[[nodiscard]]
	std::filesystem::path ?test_name::source_file()
	{
		return std::source_location::current().file_name();
	}

	void ?test_name::run_tests()
	{
		do_allocation_tests();
	}

	template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
	void ?test_name::test_allocation()
	{
		// See e.g RegularAllocationTestDiagnostics.cpp or MoveOnlyAllocationTestDiagostics.cpp
	}
}
