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
	std::string_view ?_class_false_positive_test::source_file() const noexcept
	{
		return __FILE__;
	}

	void ?_class_false_positive_test::run_tests()
	{
		// For example:
$Regular
		// ::?_class x{args}, y{different args};
		// check(equivalence, LINE("Useful Description"), x, something inequivalent - ordinarily this would fail);
		// check(equality, LINE("Useful Description"), x, y);
$Move
		// auto x = []() { return ::?_class{args}; };
		// auto y = []() { return ::?_class{different args}; };
		// check(equivalence, LINE("Useful Description"), x(), something inequivalent - ordinarily this would fail);
		// check(equality, LINE("Useful Description"), x(), y());
$
	}
}
