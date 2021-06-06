////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UniqueThingTest.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::string_view unique_thing_test::source_file() const noexcept
	{
		return __FILE__;
	}

	void unique_thing_test::run_tests()
	{
		auto x = []() { return stuff::unique_thing{0.0}; };
		auto y = []() { return stuff::unique_thing{1.0}; };

		check_equivalence(LINE(""), x(), 0.0);
		check_equivalence(LINE(""), y(), 1.0);
		check_semantics(LINE(""), x, y, std::weak_ordering::less);
	}
}
