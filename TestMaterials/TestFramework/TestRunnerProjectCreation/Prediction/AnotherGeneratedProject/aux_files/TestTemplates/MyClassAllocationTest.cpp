////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "?ClassAllocationTest.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path ?forename_?surname::source_file() const noexcept
	{
		return std::source_location::current().file_name();
	}

	void ?forename_?surname::run_tests()
	{
		do_allocation_tests(*this);
	}

	template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
	void ?forename_?surname::test_allocation()
	{
		// See e.g RegularAllocationTestDiagnostics.cpp or MoveOnlyAllocationTestDiagostics.cpp
	}
}
