////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UtilitiesFreeTest.hpp"
#include "fakeProject/Utilities/Utilities.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::string_view utilities_free_test::source_file() const noexcept
	{
		return __FILE__;
	}

	void utilities_free_test::run_tests()
	{
		// e.g. check_equality(report_line("Useful description"), some_function(), 42);
	}
}
