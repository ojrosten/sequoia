////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "BarFreeTest.hpp"
#include "fakeProject/Stuff/Bar.hpp"

/* #include "Stuff/FooTestingUtilities.hpp" */
/** #include "Stuff/FooTestingUtilities.hpp" **/
// #include "Stuff/FooTestingUtilities.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path bar_free_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void bar_free_test::run_tests()
	{
		// e.g. check_equality(report_line("Useful description"), some_function(), 42);
	}
}
