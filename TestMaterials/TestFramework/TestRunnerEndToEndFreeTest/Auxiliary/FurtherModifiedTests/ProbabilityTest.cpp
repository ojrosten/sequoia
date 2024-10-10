////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProbabilityTest.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path probability_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void probability_test::run_tests()
	{
		maths::probability x{0.5}, y{1.0};
		check(equivalence, "", x, 0.5);
		check(equivalence, "", y, 0.9);
		check_semantics("", x, y, std::weak_ordering::less);
	}
}
