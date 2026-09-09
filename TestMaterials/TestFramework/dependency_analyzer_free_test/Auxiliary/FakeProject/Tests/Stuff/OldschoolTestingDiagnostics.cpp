////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OldschoolTestingDiagnostics.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path oldschool_false_positive_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void oldschool_false_positive_test::run_tests()
	{
		// For example:

		// stuff::oldschool x{args}, y{different args};
		// check_equivalence(report_line("Useful Description"), x, something inequivalent - ordinarily this would fail);
		// check_equality(report_line("Useful Description"), x, y);
	}
}
