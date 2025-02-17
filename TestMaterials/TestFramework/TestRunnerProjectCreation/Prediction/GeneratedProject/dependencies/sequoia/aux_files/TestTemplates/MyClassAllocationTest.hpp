////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/?AllocationTestCore.hpp"

namespace ?::testing
{
	using namespace sequoia::testing;class ?forename_?surname final : public ?_allocation_test
	{
	public:
		using ?_allocation_test::?_allocation_test;

		template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
		void test_allocation();

		[[nodiscard]]
		std::filesystem::path source_file() const;

		void run_tests();
	};
}
