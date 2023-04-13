////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "?BehaviouralTest.hpp"
#include "?Header.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path ?_behavioural_test::source_file() const noexcept
	{
		return std::source_location::current().file_name();
	}

	void ?_behavioural_test::run_tests()
	{
		// e.g. check_equality(report_line("Useful description"), some_function(), 42);
	}
}
