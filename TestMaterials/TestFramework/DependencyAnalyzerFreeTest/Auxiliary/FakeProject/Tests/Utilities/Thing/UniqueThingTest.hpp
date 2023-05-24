////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UniqueThingTestingUtilities.hpp"

namespace sequoia::testing
{
	class unique_thing_test final : public move_only_test
	{
	public:
		using move_only_test::move_only_test;

	private:
		[[nodiscard]]
		std::filesystem::path source_file() const;

		void run_tests();
	};
}
