////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "?ClassTestingDiagnostics.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path ?forename_false_positive_?surname::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void ?forename_false_positive_?surname::run_tests()
	{
		// For example:
$Regular
		// ::?_class x{args}, y{different args};
		// check(equivalence, report_line("Useful Description"), x, something inequivalent - ordinarily this would fail);
		// check(equality, report_line("Useful Description"), x, y);
$Move
		// auto x = []() { return ::?_class{args}; };
		// auto y = []() { return ::?_class{different args}; };
		// check(equivalence, report_line("Useful Description"), x(), something inequivalent - ordinarily this would fail);
		// check(equality, report_line("Useful Description"), x(), y());
$
	}
}
