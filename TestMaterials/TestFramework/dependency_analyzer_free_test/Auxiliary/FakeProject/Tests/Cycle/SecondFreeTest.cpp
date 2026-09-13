////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "SecondFreeTest.hpp"
#include "fakeProject/Cycle/Second.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path second_free_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void second_free_test::run_tests()
	{
	}
}
