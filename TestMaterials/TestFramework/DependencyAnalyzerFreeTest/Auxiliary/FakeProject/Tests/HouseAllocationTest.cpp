////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "HouseAllocationTest.hpp"

/* Check that 'namespace' inside a c-style comment is ignored */
// Check that 'namespace' inside a comment is ignored

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path house_allocation_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void house_allocation_test::run_tests()
	{
		do_allocation_tests(*this);
	}

	template<bool PropagateMove, bool PropagateSwap>
	void house_allocation_test::test_allocation()
	{
		// See e.g RegularAllocationTestDiagnostics.cpp or MoveOnlyAllocationTestDiagostics.cpp
	}
}
