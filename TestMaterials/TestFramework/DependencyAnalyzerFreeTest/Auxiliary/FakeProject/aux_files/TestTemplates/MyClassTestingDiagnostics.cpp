////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "?ClassTestingDiagnostics.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path ?_class_false_positive_test::source_file() const noexcept
	{
		return std::source_location::current().file_name();
	}

	void ?_class_false_positive_test::run_tests()
	{
		// For example:
$Regular
		// ::?_class x{args}, y{different args};
		// check_equivalence(report_line("Useful Description"), x, something inequivalent - ordinarily this would fail);
		// check_equality(report_line("Useful Description"), x, y);
$Move
		// auto x = []() { return ::?_class{args}; };
		// auto y = []() { return ::?_class{different args}; };
		// check_equivalence(report_line("Useful Description"), x(), something inequivalent - ordinarily this would fail);
		// check_equality(report_line("Useful Description"), x(), y());
$
	}
}
