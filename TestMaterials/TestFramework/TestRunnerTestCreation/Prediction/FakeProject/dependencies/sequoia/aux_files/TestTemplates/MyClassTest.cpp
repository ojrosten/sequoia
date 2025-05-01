////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "?ClassTest.hpp"

namespace ?::testing
{
	[[nodiscard]]
	std::filesystem::path ?forename_?surname::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void ?forename_?surname::run_tests()
	{
		// For example:
$Regular
		// ::?_class x{args}, y{different args};
		// check(equivalence, "Useful Description", x, something equivalent);
		// check(equivalence,"Useful Description", y, something equivalent);
$Move
		// auto x = []() { return ::?_class{args}; };
		// auto y = []() { return ::?_class{different args}; };
		// check(equivalence, "Useful Description", x(), something equivalent);
		// check(equivalence, "Useful Description", y(), something equivalent);
$
		// For orderable type, with x < y:
		// check_semantics("Useful Description", x, y, std::weak_ordering::less);
		// For equality comparable but not orderable:
		// check_semantics("Useful Description", x, y);
	}
}
