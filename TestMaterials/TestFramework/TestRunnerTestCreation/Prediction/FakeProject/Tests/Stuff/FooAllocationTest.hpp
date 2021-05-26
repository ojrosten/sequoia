////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/MoveOnlyAllocationTestCore.hpp"

namespace sequoia::testing
{
	class foo_allocation_test final : public move_only_allocation_test
	{
	public:
		using move_only_allocation_test::move_only_allocation_test;

		template<bool PropagateMove, bool PropagateSwap>
		void test_allocation();

	private:
		[[nodiscard]]
		std::string_view source_file() const noexcept final;

		void run_tests() final;
	};
}
