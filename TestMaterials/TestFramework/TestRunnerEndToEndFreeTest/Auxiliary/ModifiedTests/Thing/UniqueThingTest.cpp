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
	std::filesystem::path unique_thing_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void unique_thing_test::run_tests()
	{
		auto x = []() { return stuff::unique_thing{0.0}; };
		auto y = []() { return stuff::unique_thing{1.0}; };

		check(equivalence, "", x(), 0.0);
		check(equivalence, "", y(), 1.0);
		check_semantics("", x, y, std::weak_ordering::less);
	}
}
