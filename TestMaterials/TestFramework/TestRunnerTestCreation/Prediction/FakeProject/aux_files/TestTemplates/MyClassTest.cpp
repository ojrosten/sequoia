////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "?ClassTest.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::string_view ?_class_test::source_file() const noexcept
	{
		return __FILE__;
	}

	void ?_class_test::run_tests()
	{
		// e.g ::?_class x{args}, y{different args};
		// check_equivalence(LINE("Useful Description"), x, something equivalent);
		// check_equivalence(LINE("Useful Description"), y, something equivalent);
		// For orderable type, with x < y:
		// check_semantics(LINE("Useful Description"), x, y, std::weak_ordering::less);
		// For equality comparable but not orderable:
		// check_semantics(LINE("Useful Description"), x, y);
	}
}
